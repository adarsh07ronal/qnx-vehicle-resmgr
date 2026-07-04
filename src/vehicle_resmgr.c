/* vehicle_resmgr.c
 * QNX Neutrino Resource Manager — /dev/vehicle/{speed,gear,oil_temp,door_lock}
 *
 * Architecture:
 *   ┌─────────────────────────────────────────┐
 *   │  QNX Neutrino RTOS                      │
 *   │                                         │
 *   │  CAN sim ──► vehicle_resmgr             │
 *   │              ├─ /dev/vehicle/speed      │
 *   │              ├─ /dev/vehicle/gear       │
 *   │              ├─ /dev/vehicle/oil_temp   │
 *   │              └─ /dev/vehicle/door_lock  │
 *   │                     │                  │
 *   │              vehicle_bridge             │
 *   │                     │ TCP socket        │
 *   └─────────────────────┼──────────────────┘
 *                         │ 10.0.2.2:9090 (host/Android emulator)
 *   ┌─────────────────────┼──────────────────┐
 *   │  Android AAOS       │                  │
 *   │  VHAL bridge ◄──────┘                  │
 *   │  CarPropertyManager                    │
 *   └────────────────────────────────────────┘
 *
 * Build:  see Makefile (QNX SDP 8.0, x86_64 target)
 * Run:    ./vehicle_resmgrd &
 *         cat /dev/vehicle/speed    → "72.00 km/h"
 *         cat /dev/vehicle/gear     → "DRIVE"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "vehicle_props.h"
#include "can_simulator.h"
#include "vehicle_bridge.h"

/* ── Per-device context ──────────────────────────────────────────────── */
typedef struct {
    iofunc_attr_t attr;     /* QNX mandatory: first member */
    uint32_t      prop_id;  /* which PROP_* this /dev node serves */
    char          path[64];
} VehicleAttr;

/* ── Four /dev/vehicle/* devices ─────────────────────────────────────── */
static VehicleAttr s_devs[4];
static dispatch_t *s_dpp;

/* ── Helper: format a property value as ASCII for cat/read ───────────── */
static int format_prop(uint32_t prop_id, char *buf, size_t len) {
    switch (prop_id) {
    case PROP_VEHICLE_SPEED:
        return snprintf(buf, len, "%.2f km/h\n", can_sim_get_speed());
    case PROP_GEAR_SELECTION:
        switch (can_sim_get_gear()) {
        case GEAR_PARK:    return snprintf(buf, len, "PARK\n");
        case GEAR_REVERSE: return snprintf(buf, len, "REVERSE\n");
        case GEAR_NEUTRAL: return snprintf(buf, len, "NEUTRAL\n");
        case GEAR_DRIVE:   return snprintf(buf, len, "DRIVE\n");
        default:           return snprintf(buf, len, "UNKNOWN\n");
        }
    case PROP_ENGINE_OIL_TEMP:
        return snprintf(buf, len, "%.1f C\n", can_sim_get_oil_temp());
    case PROP_DOOR_LOCK:
        return snprintf(buf, len, "%s\n", can_sim_get_door_lock() ? "LOCKED" : "UNLOCKED");
    default:
        return snprintf(buf, len, "?\n");
    }
}

/* ── io_read handler: called when userspace does read() / cat ────────── */
static int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
    VehicleAttr *attr = (VehicleAttr *)ocb->attr;
    /* static: the reply is copied out by the framework after this function
     * returns, so the backing memory for SETIOV must outlive the call. The
     * dispatch loop here is single-threaded, so a shared static is safe. */
    static char buf[64];
    int  len;

    if (ocb->offset > 0) {
        /* Already delivered data — signal EOF */
        _IO_SET_READ_NBYTES(ctp, 0);
        return _RESMGR_NPARTS(0);
    }

    len = format_prop(attr->prop_id, buf, sizeof(buf));

    SETIOV(ctp->iov, buf, len);
    ocb->offset += len;

    /* _RESMGR_NPARTS only tells the framework how many iov parts to send —
     * the actual byte count handed back to the caller's read() comes from
     * ctp->status, which this sets. Omitting it silently replies with 0
     * bytes (instant EOF) even though the iov itself has valid data. */
    _IO_SET_READ_NBYTES(ctp, len);

    return _RESMGR_NPARTS(1);
}

/* ── io_write handler: future use (e.g. set door lock from Android) ─── */
static int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb) {
    /* Accept but ignore writes for now */
    _IO_SET_WRITE_NBYTES(ctp, msg->i.nbytes);
    return _RESMGR_NPARTS(0);
}

/* ── Register one /dev/vehicle/* device ─────────────────────────────── */
static void register_device(dispatch_t *dpp, resmgr_attr_t *rattr,
                             resmgr_connect_funcs_t *connect_funcs, resmgr_io_funcs_t *io_funcs,
                             VehicleAttr *dev, const char *path, uint32_t prop_id)
{
    iofunc_attr_init(&dev->attr, S_IFCHR | 0444, NULL, NULL);
    dev->prop_id = prop_id;
    strncpy(dev->path, path, sizeof(dev->path) - 1);

    if (resmgr_attach(dpp, rattr, path, _FTYPE_ANY, 0,
                      connect_funcs, io_funcs,
                      &dev->attr) == -1) {
        fprintf(stderr, "[resmgr] attach %s failed: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("[resmgr] registered %s (prop=0x%08X)\n", path, prop_id);
}

/* ── CAN tick thread: updates state + publishes to Android ──────────── */
static void *can_tick_thread(void *arg) {
    VehicleMessage msg;
    (void)arg;

    bridge_connect();   /* best-effort; bridge_send() retries on failure */

    while (1) {
        uint32_t changed = can_sim_tick(&msg);
        if (changed) {
            bridge_send(&msg);   /* push to Android VHAL bridge */
        }
        usleep(100 * 1000);   /* 100 ms tick — matches CAN 10 Hz rate */
    }
    return NULL;
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    resmgr_attr_t      rattr;
    resmgr_connect_funcs_t connect_funcs;
    resmgr_io_funcs_t  io_funcs;
    dispatch_context_t *ctp;
    pthread_t           tick_tid;

    printf("[resmgr] QNX Vehicle Resource Manager starting\n");

    can_sim_init();

    /* ── Setup dispatch ── */
    s_dpp = dispatch_create();
    if (!s_dpp) { perror("dispatch_create"); return EXIT_FAILURE; }

    memset(&rattr,        0, sizeof(rattr));
    memset(&io_funcs,     0, sizeof(io_funcs));
    memset(&connect_funcs,0, sizeof(connect_funcs));

    /* io_read/io_write reply with a single iov part via SETIOV(ctp->iov, ...);
     * without nparts_max set, resmgr_attach() allocates zero iov capacity and
     * every read silently replies with 0 bytes (instant EOF, no error). */
    rattr.nparts_max = 1;

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS,      &io_funcs);

    /* Override read/write. Also wire up the 64-bit variants — QNX 8's
     * client-side read()/write() send _IO_READ64/_IO_WRITE64 by default,
     * and an unset .read64/.write64 silently falls back to a default
     * handler that returns EOF, so plain .read/.write alone never fires. */
    io_funcs.read    = io_read;
    io_funcs.write   = io_write;
    io_funcs.read64  = io_read;
    io_funcs.write64 = io_write;

    /* ── Register /dev/vehicle/* ── */
    mkdir("/dev/vehicle", 0755);   /* create dir if absent */

    register_device(s_dpp, &rattr, &connect_funcs, &io_funcs,
                    &s_devs[0], DEV_SPEED,    PROP_VEHICLE_SPEED);
    register_device(s_dpp, &rattr, &connect_funcs, &io_funcs,
                    &s_devs[1], DEV_GEAR,     PROP_GEAR_SELECTION);
    register_device(s_dpp, &rattr, &connect_funcs, &io_funcs,
                    &s_devs[2], DEV_OIL_TEMP, PROP_ENGINE_OIL_TEMP);
    register_device(s_dpp, &rattr, &connect_funcs, &io_funcs,
                    &s_devs[3], DEV_DOOR_LOCK, PROP_DOOR_LOCK);

    /* ── Start CAN tick thread ── */
    pthread_create(&tick_tid, NULL, can_tick_thread, NULL);

    /* ── Dispatch loop (MsgReceive) ── */
    ctp = dispatch_context_alloc(s_dpp);
    printf("[resmgr] dispatch loop running — cat /dev/vehicle/speed to test\n");

    while (1) {
        ctp = dispatch_block(ctp);
        if (!ctp) { perror("dispatch_block"); break; }
        dispatch_handler(ctp);
    }

    bridge_disconnect();
    return EXIT_SUCCESS;
}

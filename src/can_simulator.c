/* can_simulator.c
 * Simulates CAN frame ingestion — replace with real socketCAN read()
 * on hardware (vcan0 or physical CAN interface).
 *
 * On QNX with real hardware:
 *   fd = open("/dev/can0", O_RDWR);
 *   read(fd, &frame, sizeof(struct can_frame));
 *
 * Here we generate realistic vehicle data on a timer to keep the
 * project runnable in QEMU without a CAN interface.
 */

#include "can_simulator.h"
#include "vehicle_props.h"
#include <math.h>
#include <time.h>
#include <stdint.h>

/* ── Internal state ──────────────────────────────────────────────────── */
static float    s_speed_kmh   = 0.0f;
static int32_t  s_gear        = GEAR_NEUTRAL;
static float    s_oil_temp_c  = 20.0f;   /* cold start */
static uint8_t  s_door_locked = 1;

static int64_t monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* ── Public API ──────────────────────────────────────────────────────── */

void can_sim_init(void) {
    s_speed_kmh  = 0.0f;
    s_gear       = GEAR_PARK;
    s_oil_temp_c = 20.0f;
    s_door_locked = 1;
}

/*
 * can_sim_tick() — call every ~100 ms from the resmgr main loop.
 * Fills `out` with the next simulated property update.
 * Returns the PROP_* id that changed, or 0 if nothing changed.
 */
uint32_t can_sim_tick(VehicleMessage *out) {
    static int tick = 0;
    tick++;

    out->magic        = BRIDGE_MAGIC;
    out->timestamp_ms = monotonic_ms();

    /* Every 10 ticks (~1 s): cycle through properties */
    switch ((tick / 10) % 4) {

    case 0:  /* Speed ramp 0→120 km/h */
        s_speed_kmh += 2.0f;
        if (s_speed_kmh > 120.0f) s_speed_kmh = 0.0f;
        s_gear = (s_speed_kmh > 5.0f) ? GEAR_DRIVE : GEAR_PARK;

        out->prop_id  = PROP_VEHICLE_SPEED;
        out->value.f  = s_speed_kmh;
        return PROP_VEHICLE_SPEED;

    case 1:  /* Gear follows speed */
        out->prop_id  = PROP_GEAR_SELECTION;
        out->value.i  = s_gear;
        return PROP_GEAR_SELECTION;

    case 2:  /* Oil temp warms up to 90 °C */
        s_oil_temp_c += 0.5f;
        if (s_oil_temp_c > 90.0f) s_oil_temp_c = 90.0f;
        out->prop_id  = PROP_ENGINE_OIL_TEMP;
        out->value.f  = s_oil_temp_c;
        return PROP_ENGINE_OIL_TEMP;

    case 3:  /* Door lock toggle every 40 ticks */
        if (tick % 40 == 0) s_door_locked ^= 1;
        out->prop_id  = PROP_DOOR_LOCK;
        out->value.b  = s_door_locked;
        return PROP_DOOR_LOCK;

    default:
        return 0;
    }
}

/* ── Direct property reads (called by resmgr io_read handlers) ──────── */

float   can_sim_get_speed(void)     { return s_speed_kmh;   }
int32_t can_sim_get_gear(void)      { return s_gear;         }
float   can_sim_get_oil_temp(void)  { return s_oil_temp_c;  }
uint8_t can_sim_get_door_lock(void) { return s_door_locked; }

/* vehicle_props.h
 * Shared vehicle property definitions — QNX resource manager side
 * Mirrors Android VHAL property IDs (android/hardware/automotive/vehicle/VehicleProperty)
 */
#ifndef VEHICLE_PROPS_H
#define VEHICLE_PROPS_H

#include <stdint.h>

/* ── Property IDs (match Android VHAL constants) ─────────────────────── */
#define PROP_VEHICLE_SPEED      0x11600207   /* PERF_VEHICLE_SPEED    float km/h */
#define PROP_GEAR_SELECTION     0x11400400   /* GEAR_SELECTION        int32      */
#define PROP_ENGINE_OIL_TEMP    0x11600303   /* ENGINE_OIL_TEMP       float °C   */
#define PROP_DOOR_LOCK          0x11200102   /* DOOR_LOCK             bool       */

/* ── Gear enum (matches VehicleGear in VHAL) ─────────────────────────── */
typedef enum {
    GEAR_NEUTRAL  = 0,
    GEAR_REVERSE  = 1,
    GEAR_PARK     = 2,
    GEAR_DRIVE    = 4,
} VehicleGear;

/* ── Property value union ─────────────────────────────────────────────── */
typedef union {
    float   f;
    int32_t i;
    uint8_t b;
} PropValue;

/* ── Message sent over socket to Android VHAL bridge ─────────────────── */
#define BRIDGE_MAGIC  0xCAFEBABE

typedef struct __attribute__((packed)) {
    uint32_t   magic;       /* BRIDGE_MAGIC — framing guard              */
    uint32_t   prop_id;     /* one of PROP_* above                       */
    PropValue  value;       /* current value                             */
    int64_t    timestamp_ms;/* monotonic ms since QNX boot               */
} VehicleMessage;

/* ── QNX /dev paths exposed by resource manager ──────────────────────── */
#define DEV_SPEED       "/dev/vehicle/speed"
#define DEV_GEAR        "/dev/vehicle/gear"
#define DEV_OIL_TEMP    "/dev/vehicle/oil_temp"
#define DEV_DOOR_LOCK   "/dev/vehicle/door_lock"

/* ── Bridge socket (connect to Android emulator host) ────────────────── */
#define BRIDGE_HOST     "10.0.2.2"   /* QEMU host alias → Android emulator */
#define BRIDGE_PORT     9090

#endif /* VEHICLE_PROPS_H */

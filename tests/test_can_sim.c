/* tests/test_can_sim.c
 * Unit tests for the CAN simulator — runs on Linux CI without QNX.
 * Tests: initial state, tick output, property values, gear transitions.
 *
 * Build: gcc -Isrc -o test_can_sim tests/test_can_sim.c src/can_simulator.c -lm
 * Run:   ./test_can_sim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "can_simulator.h"
#include "vehicle_props.h"

static int s_pass = 0, s_fail = 0;

#define TEST(name) printf("  %-45s", name)
#define PASS()     do { printf("PASS\n"); s_pass++; } while(0)
#define FAIL(msg)  do { printf("FAIL — %s\n", msg); s_fail++; } while(0)
#define ASSERT_EQ(a,b,msg) do { if((a)==(b)) PASS(); else FAIL(msg); } while(0)
#define ASSERT_GT(a,b,msg) do { if((a)> (b)) PASS(); else FAIL(msg); } while(0)
#define ASSERT_NE(a,b,msg) do { if((a)!=(b)) PASS(); else FAIL(msg); } while(0)

/* ── Suite 1: Initial state ──────────────────────────────────────────── */
static void test_initial_state(void) {
    printf("\n[Suite 1] Initial state after can_sim_init()\n");
    can_sim_init();

    TEST("speed starts at 0");
    ASSERT_EQ(can_sim_get_speed(), 0.0f, "speed != 0");

    TEST("gear starts at PARK");
    ASSERT_EQ(can_sim_get_gear(), GEAR_PARK, "gear != PARK");

    TEST("oil temp starts at 20 C (cold)");
    ASSERT_EQ(can_sim_get_oil_temp(), 20.0f, "oil_temp != 20");

    TEST("door starts locked");
    ASSERT_EQ(can_sim_get_door_lock(), 1, "door != locked");
}

/* ── Suite 2: Message framing ────────────────────────────────────────── */
static void test_message_framing(void) {
    printf("\n[Suite 2] VehicleMessage framing\n");
    can_sim_init();
    VehicleMessage msg;

    /* tick until we get a message */
    uint32_t prop = 0;
    for (int i = 0; i < 20 && !prop; i++) prop = can_sim_tick(&msg);

    TEST("magic == BRIDGE_MAGIC");
    ASSERT_EQ(msg.magic, (uint32_t)BRIDGE_MAGIC, "magic mismatch");

    TEST("prop_id is a known property");
    int known = (msg.prop_id == PROP_VEHICLE_SPEED    ||
                 msg.prop_id == PROP_GEAR_SELECTION   ||
                 msg.prop_id == PROP_ENGINE_OIL_TEMP  ||
                 msg.prop_id == PROP_DOOR_LOCK);
    ASSERT_NE(known, 0, "unknown prop_id");

    TEST("timestamp_ms > 0");
    ASSERT_GT(msg.timestamp_ms, 0LL, "timestamp not set");
}

/* ── Suite 3: Speed ramp ─────────────────────────────────────────────── */
static void test_speed_ramp(void) {
    printf("\n[Suite 3] Speed ramp & gear transition\n");
    can_sim_init();
    VehicleMessage msg;

    /* Run enough ticks to see speed increase */
    for (int i = 0; i < 100; i++) can_sim_tick(&msg);
    float spd1 = can_sim_get_speed();

    TEST("speed increases after ticks");
    ASSERT_GT(spd1, 0.0f, "speed didn't increase");

    /* Run until speed > 5 km/h — gear should shift to DRIVE */
    for (int i = 0; i < 500 && can_sim_get_speed() <= 5.0f; i++)
        can_sim_tick(&msg);

    TEST("gear shifts to DRIVE when speed > 5 km/h");
    ASSERT_EQ(can_sim_get_gear(), GEAR_DRIVE, "gear not DRIVE");
}

/* ── Suite 4: Oil temp warm-up ───────────────────────────────────────── */
static void test_oil_temp(void) {
    printf("\n[Suite 4] Oil temperature warm-up\n");
    can_sim_init();
    VehicleMessage msg;

    float initial = can_sim_get_oil_temp();
    for (int i = 0; i < 500; i++) can_sim_tick(&msg);
    float after = can_sim_get_oil_temp();

    TEST("oil temp rises after ticks");
    ASSERT_GT(after, initial, "oil temp didn't rise");

    /* Run to saturation */
    for (int i = 0; i < 5000; i++) can_sim_tick(&msg);

    TEST("oil temp caps at 90 C");
    float final = can_sim_get_oil_temp();
    ASSERT_EQ(final <= 90.0f, 1, "oil temp exceeded 90 C");
}

/* ── Suite 5: Message size (binary protocol) ─────────────────────────── */
static void test_message_size(void) {
    printf("\n[Suite 5] Binary protocol correctness\n");

    TEST("VehicleMessage is packed (no padding)");
    /* magic(4) + prop_id(4) + value(4) + timestamp(8) = 20 bytes */
    ASSERT_EQ(sizeof(VehicleMessage), 20u, "struct size wrong — check packing");
}

/* ── main ────────────────────────────────────────────────────────────── */
int main(void) {
    printf("====================================================\n");
    printf("  QNX Vehicle Resource Manager — Unit Tests\n");
    printf("  (Linux host build — CAN simulator logic)\n");
    printf("====================================================\n");

    test_initial_state();
    test_message_framing();
    test_speed_ramp();
    test_oil_temp();
    test_message_size();

    printf("\n====================================================\n");
    printf("  Results: %d passed, %d failed\n", s_pass, s_fail);
    printf("====================================================\n");

    return s_fail ? EXIT_FAILURE : EXIT_SUCCESS;
}

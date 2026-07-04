#ifndef CAN_SIMULATOR_H
#define CAN_SIMULATOR_H

#include "vehicle_props.h"
#include <stdint.h>

void     can_sim_init(void);
uint32_t can_sim_tick(VehicleMessage *out);   /* returns changed PROP_* or 0 */

float    can_sim_get_speed(void);
int32_t  can_sim_get_gear(void);
float    can_sim_get_oil_temp(void);
uint8_t  can_sim_get_door_lock(void);
float    can_sim_get_fuel_level(void);

#endif

#ifndef VEHICLE_BRIDGE_H
#define VEHICLE_BRIDGE_H

#include "vehicle_props.h"

int  bridge_connect(void);
int  bridge_send(const VehicleMessage *msg);
void bridge_disconnect(void);

#endif

package com.adarsh.vhal;

/**
 * Mirrors android.hardware.automotive.vehicle.VehiclePropValue (AIDL) — the
 * actual VHAL wire type CarService/CarPropertyManager consume in a real AAOS
 * build. VhalBridgeService constructs one of these for every property update
 * it receives from the native bridge, so the translation this project claims
 * — "QNX IPC message -> VehiclePropValue event" — is a concrete object, not
 * just a comment.
 */
public class VehiclePropValue {

    /** VehicleProperty.* id — same PROP_* constants as vehicle_props.h. */
    public final int prop;

    /** VehicleArea.GLOBAL = 0 — this project only publishes global-scope properties. */
    public final int areaId;

    /** Elapsed-realtime nanos at the moment this event was constructed. */
    public final long timestamp;

    /** Boxed Float / Integer / Boolean payload, matching PropValue's C union. */
    public final Object value;

    public VehiclePropValue(int prop, Object value) {
        this.prop      = prop;
        this.areaId    = 0;
        this.timestamp = System.nanoTime();
        this.value     = value;
    }

    @Override
    public String toString() {
        return String.format("VehiclePropValue{prop=0x%08X, areaId=%d, value=%s, timestamp=%d}",
                prop, areaId, value, timestamp);
    }
}

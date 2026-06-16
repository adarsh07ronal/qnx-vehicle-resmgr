package com.adarsh.vhal;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import androidx.annotation.Nullable;

/**
 * VhalBridgeService
 *
 * Android service that:
 *  1. Starts the native TCP listener (via JNI) which receives VehicleMessage
 *     structs from the QNX resource manager
 *  2. Receives property callbacks from native code
 *  3. Injects values into VHAL via CarPropertyManager
 *
 * In a full AAOS build this would extend VehicleHal and run inside
 * the VHAL service process. For this demo it runs as a system app with
 * android.car.permission.CAR_ENERGY / CAR_INFO permissions.
 */
public class VhalBridgeService extends Service {

    private static final String TAG = "VhalBridgeService";

    // VHAL property IDs — match vehicle_props.h
    private static final int PROP_VEHICLE_SPEED   = 0x11600207;
    private static final int PROP_GEAR_SELECTION  = 0x11400400;
    private static final int PROP_ENGINE_OIL_TEMP = 0x11600303;
    private static final int PROP_DOOR_LOCK       = 0x11200102;

    static {
        System.loadLibrary("vhal_bridge");   // libvhal_bridge.so
    }

    /* ── Native method — starts TCP listener thread in C++ ── */
    private native void startBridge();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "Starting VHAL bridge service");
        startBridge();   // calls vhal_bridge.cpp → bridge_thread
        return START_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) { return null; }

    /* ─────────────────────────────────────────────────────────────────
     * JNI callbacks — called from bridge_thread in vhal_bridge.cpp
     * ───────────────────────────────────────────────────────────────── */

    /** Called for PROP_VEHICLE_SPEED and PROP_ENGINE_OIL_TEMP */
    public void onFloatProperty(int propId, float value) {
        String name = propId == PROP_VEHICLE_SPEED ? "SPEED" : "OIL_TEMP";
        Log.d(TAG, String.format("[QNX→VHAL] %s = %.2f", name, value));
        // In real AAOS: mCarPropertyManager.setFloatProperty(propId, 0, value);
        VhalStore.getInstance().putFloat(propId, value);
    }

    /** Called for PROP_GEAR_SELECTION */
    public void onIntProperty(int propId, int value) {
        Log.d(TAG, String.format("[QNX→VHAL] GEAR = %d", value));
        // In real AAOS: mCarPropertyManager.setIntProperty(propId, 0, value);
        VhalStore.getInstance().putInt(propId, value);
    }

    /** Called for PROP_DOOR_LOCK */
    public void onBoolProperty(int propId, boolean value) {
        Log.d(TAG, String.format("[QNX→VHAL] DOOR_LOCK = %b", value));
        // In real AAOS: mCarPropertyManager.setBooleanProperty(propId, 0, value);
        VhalStore.getInstance().putBool(propId, value);
    }
}

package com.adarsh.vhal;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * Minimal dashboard: starts VhalBridgeService (which opens the TCP listener
 * QNX's vehicle_bridge.c connects to) and polls VhalStore twice a second to
 * show the live values it receives, so the QNX -> Android data flow this
 * project is about is visible on screen, not just in logcat.
 */
public class MainActivity extends Activity {

    private TextView mStatus;
    private TextView mSpeed;
    private TextView mGear;
    private TextView mOilTemp;
    private TextView mDoorLock;
    private TextView mFuel;

    private final Handler mHandler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(48, 96, 48, 160);   // extra bottom padding clears the car system HVAC bar
        root.setGravity(Gravity.CENTER_HORIZONTAL);

        TextView title = new TextView(this);
        title.setText("QNX Vehicle Resource Manager -> Android VHAL Bridge");
        title.setTextSize(20);
        title.setPadding(0, 0, 0, 48);
        root.addView(title);

        mStatus   = addRow(root, "Bridge: waiting for QNX connection...");
        mSpeed    = addRow(root, "Speed: --");
        mGear     = addRow(root, "Gear: --");
        mOilTemp  = addRow(root, "Oil Temp: --");
        mDoorLock = addRow(root, "Door: --");
        mFuel     = addRow(root, "Fuel: --");

        ScrollView scroll = new ScrollView(this);
        scroll.addView(root);
        setContentView(scroll);

        // Foreground start from a visible Activity is always allowed, no
        // notification/foreground-service ceremony needed for this demo.
        startService(new Intent(this, VhalBridgeService.class));

        mHandler.post(mRefresh);
    }

    private TextView addRow(LinearLayout root, String initial) {
        TextView tv = new TextView(this);
        tv.setText(initial);
        tv.setTextSize(18);
        tv.setPadding(0, 16, 0, 16);
        root.addView(tv);
        return tv;
    }

    private final Runnable mRefresh = new Runnable() {
        @Override
        public void run() {
            VhalStore store = VhalStore.getInstance();

            float   speed       = store.getFloat(VhalBridgeService.PROP_VEHICLE_SPEED, -1);
            int     gear        = store.getInt(VhalBridgeService.PROP_GEAR_SELECTION, -1);
            float   oilTemp     = store.getFloat(VhalBridgeService.PROP_ENGINE_OIL_TEMP, -1);
            boolean doorLocked  = store.getBool(VhalBridgeService.PROP_DOOR_LOCK, false);
            float   fuel        = store.getFloat(VhalBridgeService.PROP_FUEL_LEVEL, -1);

            if (speed >= 0) {
                mStatus.setText("Bridge: receiving live data from QNX");
                mStatus.setTextColor(Color.parseColor("#2e7d32"));
            }

            mSpeed.setText(String.format("Speed: %.1f km/h", speed));
            mGear.setText("Gear: " + gearName(gear));
            mOilTemp.setText(String.format("Oil Temp: %.1f C", oilTemp));
            mDoorLock.setText("Door: " + (doorLocked ? "LOCKED" : "UNLOCKED"));
            mFuel.setText(String.format("Fuel: %.1f %%", fuel));

            mHandler.postDelayed(this, 500);
        }
    };

    private static String gearName(int gear) {
        switch (gear) {
            case 0:  return "NEUTRAL";
            case 1:  return "REVERSE";
            case 2:  return "PARK";
            case 4:  return "DRIVE";
            default: return "--";
        }
    }
}

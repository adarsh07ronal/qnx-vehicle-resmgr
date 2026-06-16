package com.adarsh.vhal;

import java.util.concurrent.ConcurrentHashMap;

/**
 * VhalStore — thread-safe in-memory property store.
 * Simulates the VHAL property cache that CarPropertyManager reads from.
 * In real AAOS this is replaced by the actual VHAL service IPC.
 */
public class VhalStore {

    private static final VhalStore INSTANCE = new VhalStore();
    public  static VhalStore getInstance() { return INSTANCE; }

    private final ConcurrentHashMap<Integer, Object> mProps = new ConcurrentHashMap<>();

    public void putFloat(int propId, float v)   { mProps.put(propId, v); }
    public void putInt(int propId, int v)        { mProps.put(propId, v); }
    public void putBool(int propId, boolean v)   { mProps.put(propId, v); }

    public float   getFloat(int propId, float   def) {
        Object v = mProps.get(propId); return v instanceof Float   ? (float)v   : def;
    }
    public int     getInt(int propId, int       def) {
        Object v = mProps.get(propId); return v instanceof Integer ? (int)v     : def;
    }
    public boolean getBool(int propId, boolean  def) {
        Object v = mProps.get(propId); return v instanceof Boolean ? (boolean)v : def;
    }
}

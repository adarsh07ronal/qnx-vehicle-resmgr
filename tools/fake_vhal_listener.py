#!/usr/bin/env python3
"""Stand-in for the Android VHAL bridge — listens on :9090 and decodes the
raw VehicleMessage structs vehicle_bridge.c sends, so the resmgr's TCP
publishing logic can be verified without an Android emulator.

Usage: python3 tools/fake_vhal_listener.py [port]
"""
import socket
import struct
import sys
from datetime import datetime

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 9090

# Must match the packed layout of VehicleMessage in src/vehicle_props.h:
#   uint32_t magic; uint32_t prop_id; PropValue value (4 bytes); int64_t timestamp_ms;
MSG_FORMAT = "<II4sq"
MSG_SIZE = struct.calcsize(MSG_FORMAT)
BRIDGE_MAGIC = 0xCAFEBABE

PROP_VEHICLE_SPEED   = 0x11600207
PROP_GEAR_SELECTION  = 0x11400400
PROP_ENGINE_OIL_TEMP = 0x11600303
PROP_DOOR_LOCK       = 0x11200102
PROP_FUEL_LEVEL      = 0x11600306

GEAR_NAMES = {0: "NEUTRAL", 1: "REVERSE", 2: "PARK", 4: "DRIVE"}

PROP_NAMES = {
    PROP_VEHICLE_SPEED:   "VEHICLE_SPEED",
    PROP_GEAR_SELECTION:  "GEAR_SELECTION",
    PROP_ENGINE_OIL_TEMP: "ENGINE_OIL_TEMP",
    PROP_DOOR_LOCK:       "DOOR_LOCK",
    PROP_FUEL_LEVEL:      "FUEL_LEVEL",
}


def decode_value(prop_id, raw4):
    if prop_id in (PROP_VEHICLE_SPEED, PROP_ENGINE_OIL_TEMP, PROP_FUEL_LEVEL):
        return struct.unpack("<f", raw4)[0]
    if prop_id == PROP_GEAR_SELECTION:
        gear = struct.unpack("<i", raw4)[0]
        return GEAR_NAMES.get(gear, f"UNKNOWN({gear})")
    if prop_id == PROP_DOOR_LOCK:
        return "LOCKED" if raw4[0] else "UNLOCKED"
    return raw4.hex()


def recv_exact(conn, n):
    buf = b""
    while len(buf) < n:
        chunk = conn.recv(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf


def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", PORT))
    srv.listen(1)
    print(f"[listener] waiting on :{PORT} for vehicle_bridge connections...")

    while True:
        conn, addr = srv.accept()
        print(f"[listener] connected: {addr}")
        try:
            while True:
                raw = recv_exact(conn, MSG_SIZE)
                if raw is None:
                    print("[listener] connection closed")
                    break

                magic, prop_id, value_raw, timestamp_ms = struct.unpack(MSG_FORMAT, raw)
                if magic != BRIDGE_MAGIC:
                    print(f"[listener] bad magic 0x{magic:08X}, dropping connection")
                    break

                name = PROP_NAMES.get(prop_id, f"0x{prop_id:08X}")
                value = decode_value(prop_id, value_raw)
                now = datetime.now().strftime("%H:%M:%S")
                print(f"[{now}] {name:16s} = {value}  (qnx_uptime_ms={timestamp_ms})")
        finally:
            conn.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

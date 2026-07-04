/* vhal_bridge.cpp
 * Android NDK — TCP listener that receives VehicleMessage from QNX resource
 * manager and injects values into the Android VHAL via JNI callback.
 *
 * In a real AAOS build this lives inside the VHAL HAL service process.
 * For this project it runs as a standalone NDK binary called from Java via JNI.
 *
 * Flow:
 *   QNX vehicle_resmgrd
 *       └─ TCP 9090 ──► vhal_bridge.cpp (listen)
 *                            └─ JNI ──► VhalBridgeService.java
 *                                           └─ translated into a VehiclePropValue
 *                                              (see VehiclePropValue.java) and
 *                                              cached in VhalStore, mirroring
 *                                              CarPropertyManager.set()
 */

#include <jni.h>
#include <android/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define LOG_TAG "VhalBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* ── Mirror of QNX vehicle_props.h ──────────────────────────────────── */
#define BRIDGE_MAGIC        0xCAFEBABE
#define BRIDGE_PORT         9090

#define PROP_VEHICLE_SPEED  0x11600207
#define PROP_GEAR_SELECTION 0x11400400
#define PROP_ENGINE_OIL_TEMP 0x11600303
#define PROP_DOOR_LOCK      0x11200102
#define PROP_FUEL_LEVEL     0x11600306

typedef union { float f; int32_t i; uint8_t b; } PropValue;

typedef struct __attribute__((packed)) {
    uint32_t  magic;
    uint32_t  prop_id;
    PropValue value;
    int64_t   timestamp_ms;
} VehicleMessage;

/* ── JVM globals (set on thread start) ──────────────────────────────── */
static JavaVM   *s_jvm      = nullptr;
static jobject   s_callback = nullptr;   /* VhalBridgeService instance */
static jmethodID s_onPropF  = nullptr;   /* onFloatProperty(int, float) */
static jmethodID s_onPropI  = nullptr;   /* onIntProperty(int, int)     */
static jmethodID s_onPropB  = nullptr;   /* onBoolProperty(int, boolean)*/

/* ── Bridge listener thread ──────────────────────────────────────────── */
static void *bridge_thread(void *arg) {
    (void)arg;
    JNIEnv *env = nullptr;
    s_jvm->AttachCurrentThread(&env, nullptr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(BRIDGE_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOGE("bind failed: %s", strerror(errno));
        goto done;
    }
    listen(server_fd, 1);
    LOGI("Listening for QNX on port %d", BRIDGE_PORT);

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) { LOGE("accept: %s", strerror(errno)); continue; }
        LOGI("QNX connected");

        VehicleMessage msg;
        while (true) {
            ssize_t n = recv(client, &msg, sizeof(msg), MSG_WAITALL);
            if (n != sizeof(msg)) break;
            if (msg.magic != BRIDGE_MAGIC) continue;

            /* ── Dispatch to Java via JNI ── */
            switch (msg.prop_id) {
            case PROP_VEHICLE_SPEED:
            case PROP_ENGINE_OIL_TEMP:
            case PROP_FUEL_LEVEL:
                env->CallVoidMethod(s_callback, s_onPropF,
                                    (jint)msg.prop_id, (jfloat)msg.value.f);
                break;
            case PROP_GEAR_SELECTION:
                env->CallVoidMethod(s_callback, s_onPropI,
                                    (jint)msg.prop_id, (jint)msg.value.i);
                break;
            case PROP_DOOR_LOCK:
                env->CallVoidMethod(s_callback, s_onPropB,
                                    (jint)msg.prop_id, (jboolean)msg.value.b);
                break;
            }
        }
        close(client);
        LOGI("QNX disconnected — waiting for reconnect");
    }

done:
    s_jvm->DetachCurrentThread();
    close(server_fd);
    return nullptr;
}

/* ── JNI entry points ────────────────────────────────────────────────── */
extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    s_jvm = vm;
    return JNI_VERSION_1_6;
}

/*
 * VhalBridgeService.startBridge()
 * Resolves JNI method IDs and starts the listener thread.
 */
JNIEXPORT void JNICALL
Java_com_adarsh_vhal_VhalBridgeService_startBridge(JNIEnv *env, jobject thiz) {
    s_callback = env->NewGlobalRef(thiz);

    jclass cls   = env->GetObjectClass(thiz);
    s_onPropF    = env->GetMethodID(cls, "onFloatProperty", "(IF)V");
    s_onPropI    = env->GetMethodID(cls, "onIntProperty",   "(II)V");
    s_onPropB    = env->GetMethodID(cls, "onBoolProperty",  "(IZ)V");

    pthread_t tid;
    pthread_create(&tid, nullptr, bridge_thread, nullptr);
    pthread_detach(tid);

    LOGI("VHAL bridge thread started");
}

} // extern "C"

/* vehicle_bridge.c
 * TCP socket publisher: sends VehicleMessage structs to the Android VHAL
 * bridge listener running on the Android emulator host (10.0.2.2:9090).
 *
 * In a real SA8155 system this would use:
 *   - QNX inter-VM shared memory (qvm_vdev_shmem) or
 *   - virtio channel between QNX hypervisor and Android guest VM
 * A TCP socket is functionally equivalent for emulator-based development.
 */

#include "vehicle_bridge.h"
#include "vehicle_props.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static int s_sockfd = -1;

/* ── Connect to Android emulator ─────────────────────────────────────── */
int bridge_connect(void) {
    struct sockaddr_in addr;

    s_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sockfd < 0) {
        fprintf(stderr, "[bridge] socket() failed: %s\n", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(BRIDGE_PORT);
    addr.sin_addr.s_addr = inet_addr(BRIDGE_HOST);

    if (connect(s_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[bridge] connect(%s:%d) failed: %s — will retry\n",
                BRIDGE_HOST, BRIDGE_PORT, strerror(errno));
        close(s_sockfd);
        s_sockfd = -1;
        return -1;
    }

    printf("[bridge] connected to Android VHAL bridge at %s:%d\n",
           BRIDGE_HOST, BRIDGE_PORT);
    return 0;
}

/* ── Send one property update ─────────────────────────────────────────── */
int bridge_send(const VehicleMessage *msg) {
    if (s_sockfd < 0) {
        /* attempt reconnect silently */
        if (bridge_connect() < 0) return -1;
    }

    ssize_t n = send(s_sockfd, msg, sizeof(*msg), MSG_NOSIGNAL);
    if (n != sizeof(*msg)) {
        fprintf(stderr, "[bridge] send failed (%zd): %s — disconnecting\n",
                n, strerror(errno));
        close(s_sockfd);
        s_sockfd = -1;
        return -1;
    }
    return 0;
}

void bridge_disconnect(void) {
    if (s_sockfd >= 0) {
        close(s_sockfd);
        s_sockfd = -1;
    }
}

#include "webrtc_ice.h"
#include "ui.h"

#include <string.h>
#include <stdio.h>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int webrtc_ice_get_local_endpoint(char *ip_out, int ip_size,
                                   int *port_out, int *sockfd_out) {
    if (!ip_out || !port_out || !sockfd_out) return -1;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        ui_log("[ICE] socket() failed");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = 0; /* let the kernel pick an ephemeral port */

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ui_log("[ICE] bind() failed");
        close(fd);
        return -1;
    }

    socklen_t alen = sizeof(addr);
    if (getsockname(fd, (struct sockaddr*)&addr, &alen) < 0) {
        ui_log("[ICE] getsockname() failed");
        close(fd);
        return -1;
    }
    int local_port = ntohs(addr.sin_port);

    /* Local LAN IP via nifm (network byte order, same layout as in_addr.s_addr).
       nifm is only kept initialized briefly elsewhere (main.cpp connectivity
       check), so we open/close our own handle here rather than assume it's live. */
    u32 ip = 0;
    nifmInitialize(NifmServiceType_User);
    Result rc = nifmGetCurrentIpAddress(&ip);
    nifmExit();
    if (R_FAILED(rc) || ip == 0) {
        ui_log("[ICE] nifmGetCurrentIpAddress failed: 0x%x", rc);
        close(fd);
        return -1;
    }
    struct in_addr ia;
    ia.s_addr = ip;
    char ipstr[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &ia, ipstr, sizeof(ipstr))) {
        close(fd);
        return -1;
    }

    strncpy(ip_out, ipstr, ip_size - 1);
    ip_out[ip_size - 1] = '\0';
    *port_out   = local_port;
    *sockfd_out = fd;
    return 0;
}

int webrtc_ice_create_local_candidate(const char *ufrag,
                                       char *out, int out_size,
                                       int *sockfd_out) {
    if (!ufrag || !out) return -1;

    char ipstr[INET_ADDRSTRLEN];
    int local_port = 0;
    int fd = -1;
    if (webrtc_ice_get_local_endpoint(ipstr, sizeof(ipstr), &local_port, &fd) != 0)
        return -1;

    /* WebRTC "host" candidate, priority computed per RFC8445 typical formula
       for a single host candidate (component 1, UDP). */
    char cand_line[256];
    snprintf(cand_line, sizeof(cand_line),
             "candidate:1 1 udp 2122260223 %s %d typ host generation 0 ufrag %s network-cost 999",
             ipstr, local_port, ufrag);

    snprintf(out, out_size,
             "{\"candidate\":\"%s\",\"sdpMid\":\"0\",\"sdpMLineIndex\":0,"
             "\"usernameFragment\":\"%s\"}",
             cand_line, ufrag);

    ui_log("[ICE] local cand %s:%d", ipstr, local_port);

    if (sockfd_out) *sockfd_out = fd;
    else close(fd);
    return 0;
}

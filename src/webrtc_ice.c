#include "webrtc_ice.h"
#include "ui.h"

#include <string.h>
#include <stdio.h>
#include <switch.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <mbedtls/md.h>

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

    /* Local LAN IP via nifm */
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

/* Construct an authenticated STUN Binding Request for ICE connectivity checks */
static int build_stun_binding_request(uint8_t *buf, int max_len,
                                      const char *remote_ufrag,
                                      const char *local_ufrag,
                                      const char *remote_pwd) {
    if (!buf || max_len < 128) return 0;

    memset(buf, 0, max_len);

    /* STUN Header: Type 0x0001 (Binding Request), Length (calculated below) */
    buf[0] = 0x00; buf[1] = 0x01;
    /* Magic Cookie 0x2112A442 */
    buf[4] = 0x21; buf[5] = 0x12; buf[6] = 0xA4; buf[7] = 0x42;
    /* Transaction ID (12 bytes) */
    buf[8] = 0x58; buf[9] = 0x62; buf[10] = 0x6F; buf[11] = 0x78; /* "Xbox" */
    buf[12] = 0x53; buf[13] = 0x77; buf[14] = 0x69; buf[15] = 0x74; /* "Swit" */
    buf[16] = 0x63; buf[17] = 0x68; buf[18] = 0x49; buf[19] = 0x43; /* "chIC" */

    int pos = 20;

    /* Attribute 1: USERNAME (0x0006) = "remote_ufrag:local_ufrag" */
    char username[128] = "";
    if (remote_ufrag && local_ufrag) {
        snprintf(username, sizeof(username), "%s:%s", remote_ufrag, local_ufrag);
    } else {
        snprintf(username, sizeof(username), "xbox:switch");
    }
    int ulen = (int)strlen(username);
    int ulen_pad = (ulen + 3) & ~3;

    buf[pos] = 0x00; buf[pos + 1] = 0x06;
    buf[pos + 2] = (uint8_t)(ulen >> 8); buf[pos + 3] = (uint8_t)(ulen & 0xFF);
    memcpy(buf + pos + 4, username, ulen);
    pos += 4 + ulen_pad;

    /* Attribute 2: PRIORITY (0x0024) */
    buf[pos] = 0x00; buf[pos + 1] = 0x24;
    buf[pos + 2] = 0x00; buf[pos + 3] = 0x04;
    uint32_t prio = 1853824767; /* Typical Host candidate priority */
    buf[pos + 4] = (uint8_t)(prio >> 24); buf[pos + 5] = (uint8_t)(prio >> 16);
    buf[pos + 6] = (uint8_t)(prio >> 8);  buf[pos + 7] = (uint8_t)(prio & 0xFF);
    pos += 8;

    /* Set header length before MESSAGE-INTEGRITY calculation */
    int body_len_for_hmac = pos - 20 + 24; /* +24 for MESSAGE-INTEGRITY attribute itself */
    buf[2] = (uint8_t)(body_len_for_hmac >> 8);
    buf[3] = (uint8_t)(body_len_for_hmac & 0xFF);

    /* Attribute 3: MESSAGE-INTEGRITY (0x0008) - 20 bytes HMAC-SHA1 using remote_pwd */
    if (remote_pwd && remote_pwd[0]) {
        buf[pos] = 0x00; buf[pos + 1] = 0x08;
        buf[pos + 2] = 0x00; buf[pos + 3] = 0x14; /* 20 bytes */

        const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
        if (md_info) {
            mbedtls_md_hmac(md_info,
                             (const unsigned char *)remote_pwd, strlen(remote_pwd),
                             buf, pos,
                             buf + pos + 4);
        }
        pos += 24;
    } else {
        /* Revert length if no key available */
        int body_len = pos - 20;
        buf[2] = (uint8_t)(body_len >> 8);
        buf[3] = (uint8_t)(body_len & 0xFF);
    }

    return pos;
}

int webrtc_ice_probe_remote_port(int sockfd, const char *remote_ip, int default_port,
                                  const char *remote_ufrag, const char *local_ufrag,
                                  const char *remote_pwd) {
    if (sockfd < 0 || !remote_ip) return default_port;

    static const int probe_ports[] = { 10257, 5050, 9002, 3074, 5000, 49152, 1024, 1025 };
    int num_ports = sizeof(probe_ports) / sizeof(probe_ports[0]);

    uint8_t stun_req_auth[256];
    int stun_auth_len = build_stun_binding_request(stun_req_auth, sizeof(stun_req_auth),
                                                   remote_ufrag, local_ufrag, remote_pwd);

    /* Plain 20-byte STUN Binding Request */
    static const uint8_t stun_req_raw[20] = {
        0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xA4, 0x42,
        0x58, 0x62, 0x6F, 0x78, 0x53, 0x77, 0x69, 0x74, 0x63, 0x68, 0x49, 0x43
    };

    ui_log("[STUN] Probing Xbox IP %s ports (10257, 5050, 9002, 3074, 5000)...", remote_ip);

    struct timeval tv = { 0, 100000 }; /* 100ms timeout */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int active_port = default_port;

    for (int pass = 0; pass < 3; pass++) {
        for (int i = 0; i < num_ports; i++) {
            int port = probe_ports[i];

            struct sockaddr_in dest;
            memset(&dest, 0, sizeof(dest));
            dest.sin_family = AF_INET;
            dest.sin_port = htons((uint16_t)port);
            inet_pton(AF_INET, remote_ip, &dest.sin_addr);

            /* Send both raw and authenticated STUN packets */
            sendto(sockfd, stun_req_raw, sizeof(stun_req_raw), 0, (struct sockaddr *)&dest, sizeof(dest));
            sendto(sockfd, stun_req_auth, stun_auth_len, 0, (struct sockaddr *)&dest, sizeof(dest));

            /* Check for reply */
            uint8_t buf[512];
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            ssize_t ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
            if (ret > 0) {
                active_port = ntohs(from.sin_port);
                ui_log("[STUN] Xbox STUN reply from %s:%d! (pass %d)", remote_ip, active_port, pass + 1);
                return active_port;
            }
        }
        usleep(30000);
    }

    ui_log("[STUN] No reply on probe ports, using default port %d", active_port);
    return active_port;
}

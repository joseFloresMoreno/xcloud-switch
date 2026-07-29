#ifndef XCLOUD_WEBRTC_CLIENT_H
#define XCLOUD_WEBRTC_CLIENT_H

#include <stdint.h>

typedef struct {
    uint8_t client_master_key[16];
    uint8_t server_master_key[16];
    uint8_t client_master_salt[14];
    uint8_t server_master_salt[14];
    int valid;
} SRTPKeys;

typedef struct {
    char local_sdp[4096];
    char remote_sdp[4096];
    char connection_state[32];
    int is_connected;
    int sockfd;
    SRTPKeys srtp_keys;
    void* dtls_ssl_ctx;   /* Opaque pointer to mbedtls_ssl_context */
    void* dtls_conf_ctx;  /* Opaque pointer to mbedtls_ssl_config */
} XCloudWebRTCClient;

// WebRTC functions
XCloudWebRTCClient* webrtc_create(void);
void webrtc_destroy(XCloudWebRTCClient* client);
int webrtc_create_offer(XCloudWebRTCClient* client, char* sdp_out, int sdp_out_size);
int webrtc_set_remote_sdp(XCloudWebRTCClient* client, const char* sdp);
int webrtc_add_ice_candidate(XCloudWebRTCClient* client, const char* candidate);
int webrtc_is_connected(XCloudWebRTCClient* client);
void webrtc_send_input(XCloudWebRTCClient* client, const char* data, int size);

// DTLS & SRTP functions
int webrtc_dtls_connect(XCloudWebRTCClient* client, int sockfd, const char* remote_ip, int remote_port);

#endif // XCLOUD_WEBRTC_CLIENT_H

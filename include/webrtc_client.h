#ifndef XCLOUD_WEBRTC_CLIENT_H
#define XCLOUD_WEBRTC_CLIENT_H

typedef struct {
    char local_sdp[4096];
    char remote_sdp[4096];
    char connection_state[32];
    int is_connected;
} XCloudWebRTCClient;

// WebRTC functions
XCloudWebRTCClient* webrtc_create(void);
void webrtc_destroy(XCloudWebRTCClient* client);
int webrtc_create_offer(XCloudWebRTCClient* client, char* sdp_out, int sdp_out_size);
int webrtc_set_remote_sdp(XCloudWebRTCClient* client, const char* sdp);
int webrtc_add_ice_candidate(XCloudWebRTCClient* client, const char* candidate);
int webrtc_is_connected(XCloudWebRTCClient* client);
void webrtc_send_input(XCloudWebRTCClient* client, const char* data, int size);

#endif // XCLOUD_WEBRTC_CLIENT_H

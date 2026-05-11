#ifndef XCLOUD_API_H
#define XCLOUD_API_H

typedef enum {
    XCLOUD_TYPE_HOME,
    XCLOUD_TYPE_CLOUD,
} XCloudType;

typedef struct {
    char* token;
    char host[256];
    XCloudType type;
} XCloudApiClient;

// API Client functions
XCloudApiClient* xcloud_api_create(const char* host, const char* token, XCloudType type);
void xcloud_api_destroy(XCloudApiClient* client);
int xcloud_api_get_titles(XCloudApiClient* client, char* response, int response_size);
int xcloud_api_start_stream(XCloudApiClient* client, const char* target, char* response, int response_size);
int xcloud_api_stop_stream(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_get_stream_state(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_send_sdp(XCloudApiClient* client, const char* session_id, const char* sdp, char* response, int response_size);
int xcloud_api_send_ice(XCloudApiClient* client, const char* session_id, const char* ice, char* response, int response_size);
int xcloud_api_send_keepalive(XCloudApiClient* client, const char* session_id, char* response, int response_size);

#endif // XCLOUD_API_H

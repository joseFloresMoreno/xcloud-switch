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
    void* curl_handle;
} XCloudApiClient;

// API Client functions
XCloudApiClient* xcloud_api_create(const char* host, const char* token, XCloudType type);
void xcloud_api_destroy(XCloudApiClient* client);
int xcloud_api_get_titles(XCloudApiClient* client, char* response, int response_size);
int xcloud_api_start_stream(XCloudApiClient* client, const char* target, char* response, int response_size);
int xcloud_api_stop_stream(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_get_stream_state(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_send_sdp(XCloudApiClient* client, const char* session_id, const char* sdp, char* response, int response_size);
int xcloud_api_get_sdp(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_send_ice(XCloudApiClient* client, const char* session_id, const char* ice_candidate_json, char* response, int response_size);
int xcloud_api_get_ice(XCloudApiClient* client, const char* session_id, char* response, int response_size);
int xcloud_api_send_keepalive(XCloudApiClient* client, const char* session_id, char* response, int response_size);

/* Send MSAL userToken to authenticate with the Xbox console (ReadyToConnect state).
   access_token is the MSA OAuth access token from xbox_auth. */
int xcloud_api_send_msal_auth(XCloudApiClient* client, const char* session_id,
                               const char* access_token, char* response, int response_size);

/* Parse the session UUID from a start_stream response body.
   Looks for "sessionPath": "/v5/sessions/{uuid}" and extracts the UUID.
   Returns 0 on success, -1 if not found. */
int xcloud_api_parse_session_id(const char* response, char* out, int out_size);

#endif // XCLOUD_API_H

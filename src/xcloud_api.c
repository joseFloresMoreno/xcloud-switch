#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "xcloud_api.h"

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

/* ------------------------------------------------------------------ */
/* Shared utilities                                                     */
/* ------------------------------------------------------------------ */

static const char* type_str(XCloudType type) {
    return (type == XCLOUD_TYPE_HOME) ? "home" : "cloud";
}

/* ------------------------------------------------------------------ */
/* curl implementation                                                  */
/* ------------------------------------------------------------------ */

#ifdef HAVE_CURL

typedef struct {
    char* buf;
    int   len;
    int   cap;
} WriteBuffer;

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userp) {
    size_t bytes = size * nmemb;
    WriteBuffer* wb = (WriteBuffer*)userp;
    if (wb->len + (int)bytes + 1 > wb->cap) {
        fprintf(stderr, "[API] response buffer overflow\n");
        return 0;
    }
    memcpy(wb->buf + wb->len, ptr, bytes);
    wb->len += (int)bytes;
    wb->buf[wb->len] = '\0';
    return bytes;
}

/* Generic HTTP request.
   method   : "GET", "POST", "DELETE"
   path     : URL path (e.g. "/v2/titles")
   body     : JSON body for POST, or NULL
   response : output buffer, may be NULL for DELETE
   Returns 0 on HTTP 2xx, negative on error. */
static int do_request(XCloudApiClient* client,
                      const char* method,
                      const char* path,
                      const char* body,
                      char* response,
                      int   response_size) {
    CURL* curl = (CURL*)client->curl_handle;
    if (!curl) return -1;

    char url[512];
    snprintf(url, sizeof(url), "https://%s%s", client->host, path);

    /* gsToken JWTs can be 1000-3000+ chars — buffer must be large enough */
    char auth_header[4096 + 32];
    snprintf(auth_header, sizeof(auth_header),
             "Authorization: Bearer %s", client->token);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "x-ms-client-version: 104.1004.18.0");

    char  null_buf[1] = {'\0'};
    WriteBuffer wb = {
        response ? response : null_buf,
        0,
        response ? response_size - 1 : 0
    };

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    /* TODO: replace with proper CA bundle path for production */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (strcmp(method, "POST") == 0) {
        const char* post_body = body ? body : "{}";
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(post_body));
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        fprintf(stderr, "[API] curl error (%s %s): %s\n",
                method, path, curl_easy_strerror(res));
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("[API] %s %s -> HTTP %ld\n", method, path, http_code);

    return (http_code >= 200 && http_code < 300) ? 0 : (int)http_code;
}

XCloudApiClient* xcloud_api_create(const char* host, const char* token, XCloudType type) {
    XCloudApiClient* client = (XCloudApiClient*)malloc(sizeof(XCloudApiClient));
    if (!client) return NULL;

    memset(client, 0, sizeof(XCloudApiClient));
    client->token = (char*)malloc(strlen(token) + 1);
    if (!client->token) { free(client); return NULL; }
    strcpy(client->token, token);
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->type = type;

    curl_global_init(CURL_GLOBAL_ALL);
    client->curl_handle = curl_easy_init();
    if (!client->curl_handle) {
        fprintf(stderr, "[API] failed to init curl handle\n");
        free(client->token);
        free(client);
        return NULL;
    }

    printf("[API] client ready (curl) — host=%s type=%s\n", host, type_str(type));
    return client;
}

void xcloud_api_destroy(XCloudApiClient* client) {
    if (!client) return;
    if (client->curl_handle) {
        curl_easy_cleanup((CURL*)client->curl_handle);
        curl_global_cleanup();
    }
    free(client->token);
    free(client);
}

int xcloud_api_get_titles(XCloudApiClient* client, char* response, int response_size) {
    if (!client || !response) return -1;
    return do_request(client, "GET", "/v2/titles", NULL, response, response_size);
}

int xcloud_api_start_stream(XCloudApiClient* client, const char* target,
                             char* response, int response_size) {
    if (!client || !target || !response) return -1;

    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/play", type_str(client->type));

    /* HOME uses serverId (console's id from xccs), CLOUD uses titleId */
    const int is_home = (client->type == XCLOUD_TYPE_HOME);
    char body[1024];
    snprintf(body, sizeof(body),
             "{"
             "\"titleId\":\"%s\","
             "\"systemUpdateGroup\":\"\","
             "\"clientSessionId\":\"\","
             "\"settings\":{"
             "\"nanoVersion\":\"V3;WebrtcTransport.dll\","
             "\"enableTextToSpeech\":false,"
             "\"highContrast\":0,"
             "\"locale\":\"en-US\","
             "\"useIceConnection\":false,"
             "\"timezoneOffsetMinutes\":120,"
             "\"sdkType\":\"web\","
             "\"osName\":\"windows\""
             "},"
             "\"serverId\":\"%s\","
             "\"fallbackRegionNames\":[]"
             "}",
             is_home ? "" : target,   /* titleId: empty for home */
             is_home ? target : "");  /* serverId: console id for home */

    /* Device info header required by the streaming API */
    static const char device_info[] =
        "{\"appInfo\":{\"env\":{\"clientAppId\":\"Microsoft.GamingApp\","
        "\"clientAppType\":\"native\",\"clientAppVersion\":\"2203.1001.4.0\","
        "\"clientSdkVersion\":\"8.5.2\",\"httpEnvironment\":\"prod\",\"sdkInstallId\":\"\"}},"
        "\"dev\":{\"hw\":{\"make\":\"Microsoft\",\"model\":\"Surface Pro\",\"sdktype\":\"native\"},"
        "\"os\":{\"name\":\"Windows 11\",\"ver\":\"22631.2715\",\"platform\":\"desktop\"},"
        "\"displayInfo\":{\"dimensions\":{\"widthInPixels\":1920,\"heightInPixels\":1080},"
        "\"pixelDensity\":{\"dpiX\":1,\"dpiY\":1}}}}";

    CURL* curl = (CURL*)client->curl_handle;
    if (!curl) return -1;

    char url[512];
    snprintf(url, sizeof(url), "https://%s%s", client->host, path);

    char auth_header[4096 + 32];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", client->token);

    char device_header[2048];
    snprintf(device_header, sizeof(device_header), "X-MS-Device-Info: %s", device_info);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, device_header);

    char null_buf[1] = {'\0'};
    WriteBuffer wb = { response ? response : null_buf, 0, response ? response_size - 1 : 0 };

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        fprintf(stderr, "[API] start_stream curl error: %s\n", curl_easy_strerror(res));
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("[API] POST %s -> HTTP %ld\n", path, http_code);

    return (http_code >= 200 && http_code < 300) ? 0 : (int)http_code;
}

int xcloud_api_stop_stream(XCloudApiClient* client, const char* session_id,
                            char* response, int response_size) {
    if (!client || !session_id) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s",
             type_str(client->type), session_id);
    return do_request(client, "DELETE", path, NULL, response, response_size);
}

int xcloud_api_get_stream_state(XCloudApiClient* client, const char* session_id,
                                 char* response, int response_size) {
    if (!client || !session_id || !response) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/state",
             type_str(client->type), session_id);
    return do_request(client, "GET", path, NULL, response, response_size);
}

int xcloud_api_send_sdp(XCloudApiClient* client, const char* session_id,
                         const char* sdp, char* response, int response_size) {
    if (!client || !session_id || !sdp) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/sdp",
             type_str(client->type), session_id);
    /* Body format per Greenlight xcloudapi.ts sendSdp() */
    int body_size = (int)strlen(sdp) + 512;
    char* body = (char*)malloc(body_size);
    if (!body) return -1;
    snprintf(body, body_size,
             "{"
               "\"messageType\":\"offer\","
               "\"sdp\":\"%s\","
               "\"configuration\":{"
                 "\"chatConfiguration\":{"
                   "\"bytesPerSample\":2,"
                   "\"expectedClipDurationMs\":20,"
                   "\"format\":{\"codec\":\"opus\",\"container\":\"webm\"},"
                   "\"numChannels\":1,"
                   "\"sampleFrequencyHz\":24000"
                 "},"
                 "\"chat\":{\"minVersion\":1,\"maxVersion\":1},"
                 "\"control\":{\"minVersion\":1,\"maxVersion\":3},"
                 "\"input\":{\"minVersion\":1,\"maxVersion\":8},"
                 "\"message\":{\"minVersion\":1,\"maxVersion\":1}"
               "}"
             "}",
             sdp);
    int ret = do_request(client, "POST", path, body, response, response_size);
    free(body);
    return ret;
}

int xcloud_api_get_sdp(XCloudApiClient* client, const char* session_id,
                        char* response, int response_size) {
    if (!client || !session_id || !response) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/sdp",
             type_str(client->type), session_id);
    return do_request(client, "GET", path, NULL, response, response_size);
}

int xcloud_api_send_ice(XCloudApiClient* client, const char* session_id,
                         const char* ice_candidate_json, char* response, int response_size) {
    if (!client || !session_id || !ice_candidate_json) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/ice",
             type_str(client->type), session_id);
    /* Body format per Greenlight sendIce(): {messageType, candidate} object */
    int body_size = (int)strlen(ice_candidate_json) + 32;
    char* body = (char*)malloc(body_size);
    if (!body) return -1;
    snprintf(body, body_size, "{\"messageType\":\"iceCandidate\",\"candidate\":%s}",
             ice_candidate_json);
    int ret = do_request(client, "POST", path, body, response, response_size);
    free(body);
    return ret;
}

int xcloud_api_get_ice(XCloudApiClient* client, const char* session_id,
                        char* response, int response_size) {
    if (!client || !session_id || !response) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/ice",
             type_str(client->type), session_id);
    return do_request(client, "GET", path, NULL, response, response_size);
}

int xcloud_api_send_keepalive(XCloudApiClient* client, const char* session_id,
                               char* response, int response_size) {
    if (!client || !session_id) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/keepalive",
             type_str(client->type), session_id);
    return do_request(client, "POST", path, "{}", response, response_size);
}

int xcloud_api_send_msal_auth(XCloudApiClient* client, const char* session_id,
                               const char* access_token,
                               char* response, int response_size) {
    if (!client || !session_id || !access_token) return -1;
    char path[256];
    snprintf(path, sizeof(path), "/v5/sessions/%s/%s/connect",
             type_str(client->type), session_id);
    int body_len = (int)strlen(access_token) + 32;
    char* body = (char*)malloc(body_len);
    if (!body) return -1;
    snprintf(body, body_len, "{\"userToken\":\"%s\"}", access_token);
    int ret = do_request(client, "POST", path, body, response, response_size);
    free(body);
    return ret;
}

int xcloud_api_parse_session_id(const char* response, char* out, int out_size) {
    if (!response || !out || out_size <= 0) return -1;
    /* Find "sessionPath" key, then extract last path component.
     * sessionPath format: /v5/sessions/{uuid} — uuid is component [3] */
    const char* p = strstr(response, "\"sessionPath\"");
    if (!p) return -1;
    p = strchr(p, '/');
    if (!p) return -1;
    /* Find the end of the path string (closing '"') */
    const char* path_end = strchr(p, '"');
    if (!path_end) return -1;
    /* Find the LAST '/' before path_end */
    const char* last_slash = p;
    const char* scan = p + 1;
    while (scan < path_end) {
        if (*scan == '/') last_slash = scan;
        scan++;
    }
    last_slash++;  /* skip '/' */
    size_t len = (size_t)(path_end - last_slash);
    if (len == 0 || len >= (size_t)out_size) return -1;
    memcpy(out, last_slash, len);
    out[len] = '\0';
    return 0;
}

#else  /* !HAVE_CURL — stubs until switch-curl is installed */

XCloudApiClient* xcloud_api_create(const char* host, const char* token, XCloudType type) {
    XCloudApiClient* client = (XCloudApiClient*)malloc(sizeof(XCloudApiClient));
    if (!client) return NULL;
    memset(client, 0, sizeof(XCloudApiClient));
    client->token = (char*)malloc(strlen(token) + 1);
    if (!client->token) { free(client); return NULL; }
    strcpy(client->token, token);
    strncpy(client->host, host, sizeof(client->host) - 1);
    client->type = type;
    printf("[API] client ready (stub, no curl) — host=%s type=%s\n",
           host, type_str(type));
    return client;
}

void xcloud_api_destroy(XCloudApiClient* client) {
    if (!client) return;
    free(client->token);
    free(client);
}

int xcloud_api_get_titles(XCloudApiClient* client, char* response, int response_size) {
    (void)client;
    if (response && response_size > 0)
        snprintf(response, response_size, "{\"titles\":[]}");
    printf("[API] get_titles: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_start_stream(XCloudApiClient* client, const char* target,
                             char* response, int response_size) {
    (void)client; (void)target;
    if (response && response_size > 0)
        snprintf(response, response_size, "{}");
    printf("[API] start_stream: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_stop_stream(XCloudApiClient* client, const char* session_id,
                            char* response, int response_size) {
    (void)client; (void)session_id; (void)response; (void)response_size;
    printf("[API] stop_stream: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_get_stream_state(XCloudApiClient* client, const char* session_id,
                                 char* response, int response_size) {
    (void)client; (void)session_id;
    if (response && response_size > 0)
        snprintf(response, response_size, "{\"state\":\"pending\"}");
    printf("[API] get_stream_state: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_send_sdp(XCloudApiClient* client, const char* session_id,
                         const char* sdp, char* response, int response_size) {
    (void)client; (void)session_id; (void)sdp; (void)response; (void)response_size;
    printf("[API] send_sdp: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_send_ice(XCloudApiClient* client, const char* session_id,
                         const char* ice, char* response, int response_size) {
    (void)client; (void)session_id; (void)ice; (void)response; (void)response_size;
    printf("[API] send_ice: stub (install switch-curl)\n");
    return -1;
}

int xcloud_api_send_keepalive(XCloudApiClient* client, const char* session_id,
                               char* response, int response_size) {
    (void)client; (void)session_id; (void)response; (void)response_size;
    printf("[API] send_keepalive: stub\n");
    return -1;
}

int xcloud_api_send_msal_auth(XCloudApiClient* client, const char* session_id,
                               const char* access_token,
                               char* response, int response_size) {
    (void)client; (void)session_id; (void)access_token; (void)response; (void)response_size;
    printf("[API] send_msal_auth: stub\n");
    return -1;
}

int xcloud_api_parse_session_id(const char* response, char* out, int out_size) {
    (void)response; (void)out; (void)out_size;
    return -1;
}

#endif /* HAVE_CURL */

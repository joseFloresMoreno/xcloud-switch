#include "xbox_host.h"
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

/* ------------------------------------------------------------------ */
/* Lifecycle                                                            */
/* ------------------------------------------------------------------ */

XboxHostList* xbox_host_list_create(void) {
    XboxHostList* list = (XboxHostList*)calloc(1, sizeof(XboxHostList));
    return list;
}

void xbox_host_list_destroy(XboxHostList* list) {
    free(list);
}

/* ------------------------------------------------------------------ */
/* Xbox Live console list (xccs.xboxlive.com)                          */
/* ------------------------------------------------------------------ */

#ifdef HAVE_CURL

typedef struct { char* buf; size_t len; size_t cap; } DynBuf;

static size_t dynbuf_write(void* ptr, size_t size, size_t nmemb, void* ud) {
    size_t bytes = size * nmemb;
    DynBuf* b = (DynBuf*)ud;
    if (b->len + bytes + 1 > b->cap) {
        size_t nc = b->cap ? b->cap * 2 : 4096;
        while (nc < b->len + bytes + 1) nc *= 2;
        char* t = (char*)realloc(b->buf, nc);
        if (!t) return 0;
        b->buf = t; b->cap = nc;
    }
    memcpy(b->buf + b->len, ptr, bytes);
    b->len += bytes;
    b->buf[b->len] = '\0';
    return bytes;
}

/* Map Xbox powerState string to XboxHostState */
static XboxHostState map_power_state(const char* ps) {
    if (!ps) return XBOX_HOST_STATE_UNKNOWN;
    if (strstr(ps, "On"))               return XBOX_HOST_STATE_AVAILABLE;
    if (strstr(ps, "ConnectedStandby")) return XBOX_HOST_STATE_AVAILABLE;
    if (strstr(ps, "Off"))              return XBOX_HOST_STATE_OFFLINE;
    return XBOX_HOST_STATE_UNKNOWN;
}

/*
 * Parse the xccs console list JSON.
 * The relevant fragment looks like (simplified):
 *   "result":[{"id":"...","name":"...","powerState":"On","consoleType":"XboxOneX",...},...]
 *
 * We iterate each object in the result array manually without a JSON library.
 */
static void parse_console_list(const char* json, XboxHostList* list) {
    const char* result = strstr(json, "\"result\":[");
    if (!result) {
        fprintf(stderr, "[CONSOLES] 'result' array not found in response\n");
        return;
    }

    const char* p = result + strlen("\"result\":[");

    while (*p && list->count < XBOX_MAX_HOSTS) {
        /* Find next '{' that starts a console object */
        p = strchr(p, '{');
        if (!p) break;

        /* Find matching closing '}' — naive single-level scan */
        const char* obj_end = strchr(p + 1, '}');
        if (!obj_end) break;

        /* Carve out the object substring */
        size_t obj_len = (size_t)(obj_end - p) + 1;
        if (obj_len >= 2048) { p = obj_end + 1; continue; }

        char obj[2048];
        memcpy(obj, p, obj_len);
        obj[obj_len] = '\0';

        /* Extract fields */
        char id[XBOX_HOST_ID_MAX]   = {0};
        char name[XBOX_HOST_NAME_MAX] = {0};
        char pstate[32]             = {0};
        char ctype[32]              = {0};
        char streaming[8]           = {0};

        /* Helper: find "key":"value" inside obj */
#define EXTRACT(key, buf, bufsz) do { \
    const char* _k = strstr(obj, "\"" key "\":\""); \
    if (_k) { \
        _k += strlen("\"" key "\":\""); \
        const char* _e = strchr(_k, '"'); \
        if (_e) { size_t _l = (size_t)(_e-_k); \
            if (_l >= (bufsz)) _l = (bufsz)-1; \
            memcpy((buf), _k, _l); (buf)[_l] = '\0'; } \
    } } while(0)

        EXTRACT("id",                  id,        sizeof(id));
        EXTRACT("name",                name,      sizeof(name));
        EXTRACT("powerState",          pstate,    sizeof(pstate));
        EXTRACT("consoleType",         ctype,     sizeof(ctype));
        EXTRACT("consoleStreamingEnabled", streaming, sizeof(streaming));
#undef EXTRACT

        if (id[0] && name[0]) {
            XboxHost* h = &list->hosts[list->count];
            snprintf(h->name, sizeof(h->name), "%s", name);
            snprintf(h->id,   sizeof(h->id),   "%s", id);
            h->ip[0]    = '\0';   /* IP is discovered locally or via teredo */
            h->state    = map_power_state(pstate);
            h->is_manual = 0;

            printf("[CONSOLES] Found: %s (%s) power=%s streaming=%s\n",
                   name, ctype, pstate, streaming);
            list->count++;
        }

        p = obj_end + 1;
    }
}

#endif /* HAVE_CURL */

/*
 * Fetch the list of Xbox consoles registered to the account.
 * Requires a valid XSTS token (http://xboxlive.com relying party).
 *
 * Endpoint: GET https://xccs.xboxlive.com/lists/devices
 *           ?queryCurrentDevice=false&includeStorageDevices=true
 * Auth:     XBL3.0 x=<uhs>;<xsts_token>
 */
int xbox_host_fetch_from_api(XboxHostList* list,
                             const char* uhs,
                             const char* xsts_token) {
    if (!list || !uhs || !xsts_token) return -1;

#ifndef HAVE_CURL
    fprintf(stderr, "[CONSOLES] switch-curl not installed\n");
    return -1;
#else
    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    DynBuf resp = {NULL, 0, 0};

    char auth[4096 + 128];
    snprintf(auth, sizeof(auth), "Authorization: XBL3.0 x=%s;%s", uhs, xsts_token);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, auth);
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "x-xbl-contract-version: 4");

    curl_easy_setopt(curl, CURLOPT_URL,
        "https://xccs.xboxlive.com/lists/devices"
        "?queryCurrentDevice=false&includeStorageDevices=true");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dynbuf_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || !resp.buf) {
        ui_log("[CONSOLES] curl err: %s (HTTP %ld)", curl_easy_strerror(res), http_code);
        free(resp.buf);
        return -1;
    }

    ui_log("[CONSOLES] HTTP %ld -- %.80s", http_code,
           resp.buf ? resp.buf : "(empty body)");

    if (http_code == 200 && resp.buf) {
        parse_console_list(resp.buf, list);
        ui_log("[CONSOLES] %d consola(s) encontrada(s)", list->count);
    } else {
        ui_log("[CONSOLES] Error %ld", http_code);
        free(resp.buf);
        return -1;
    }

    free(resp.buf);
    return list->count;
#endif
}

/* ------------------------------------------------------------------ */
/* mDNS local discovery (stub)                                         */
/* ------------------------------------------------------------------ */

/*
 * TODO: implement mDNS scan for _xbox-smartglass._tcp.local
 * Xbox consoles broadcast on UDP multicast 224.0.0.251:5353
 * Reference: https://github.com/OpenXbox/xbox-smartglass-core-python
 */
int xbox_host_discover(XboxHostList* list) {
    if (!list) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Manual IP entry                                                      */
/* ------------------------------------------------------------------ */

int xbox_host_add_manual(XboxHostList* list, const char* name, const char* ip) {
    if (!list || list->count >= XBOX_MAX_HOSTS) return -1;
    XboxHost* h = &list->hosts[list->count];
    snprintf(h->name, sizeof(h->name), "%s", name);
    snprintf(h->ip,   sizeof(h->ip),   "%s", ip);
    snprintf(h->id,   sizeof(h->id),   "manual-%d", list->count);
    h->state     = XBOX_HOST_STATE_UNKNOWN;
    h->is_manual = 1;
    list->count++;
    return list->count - 1;
}

int xbox_host_load_config(XboxHostList* list, const char* filepath) {
    if (!list || !filepath) return -1;
    FILE* f = fopen(filepath, "r");
    if (!f) {
        /* Ensure directory exists and create a template config file */
        mkdir("/switch", 0777);
        mkdir("/switch/greenlight", 0777);
        FILE* wf = fopen(filepath, "w");
        if (wf) {
            fprintf(wf, "# Greenlight Switch Configuration\n");
            fprintf(wf, "# Escribe la IP de tu Xbox local a continuacion (ej. ip=192.168.1.100)\n");
            fprintf(wf, "ip=\n");
            fclose(wf);
            ui_log("[CONSOLES] Plantilla de config creada en %s", filepath);
        }
        return -1;
    }

    char line[128];
    char config_ip[64] = "";
    while (fgets(line, sizeof(line), f)) {
        /* Strip newline/CR */
        char* p = line;
        while (*p) {
            if (*p == '\r' || *p == '\n') *p = '\0';
            p++;
        }
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "ip=", 3) == 0) {
            strncpy(config_ip, line + 3, sizeof(config_ip) - 1);
        } else if (strstr(line, ".") != NULL && config_ip[0] == '\0') {
            /* Raw IP address in file */
            strncpy(config_ip, line, sizeof(config_ip) - 1);
        }
    }
    fclose(f);

    if (config_ip[0] != '\0') {
        ui_log("[CONSOLES] Config IP cargada: %s", config_ip);
        /* Assign IP to any console lacking an IP */
        for (int i = 0; i < list->count; i++) {
            if (list->hosts[i].ip[0] == '\0') {
                strncpy(list->hosts[i].ip, config_ip, sizeof(list->hosts[i].ip) - 1);
            }
        }
        /* If list is empty, add manual host */
        if (list->count == 0) {
            xbox_host_add_manual(list, "Xbox (Config IP)", config_ip);
        }
        return 0;
    }
    return -1;
}

void xbox_host_list_clear(XboxHostList* list) {
    if (!list) return;
    list->count = 0;
    memset(list->hosts, 0, sizeof(list->hosts));
}

const char* xbox_host_state_string(XboxHostState state) {
    switch (state) {
        case XBOX_HOST_STATE_AVAILABLE: return "Disponible";
        case XBOX_HOST_STATE_BUSY:      return "Ocupado";
        case XBOX_HOST_STATE_OFFLINE:   return "Sin conexion";
        default:                        return "Desconocido";
    }
}

const char* xbox_host_state_string_en(XboxHostState state) {
    switch (state) {
        case XBOX_HOST_STATE_AVAILABLE: return "Online";
        case XBOX_HOST_STATE_BUSY:      return "Busy";
        case XBOX_HOST_STATE_OFFLINE:   return "Offline";
        default:                        return "Unknown";
    }
}

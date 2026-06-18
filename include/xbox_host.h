#ifndef XBOX_HOST_H
#define XBOX_HOST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XBOX_HOST_NAME_MAX  64
#define XBOX_HOST_IP_MAX    64
#define XBOX_HOST_ID_MAX    128
#define XBOX_MAX_HOSTS      16

typedef enum {
    XBOX_HOST_STATE_UNKNOWN = 0,
    XBOX_HOST_STATE_AVAILABLE,
    XBOX_HOST_STATE_BUSY,
    XBOX_HOST_STATE_OFFLINE,
} XboxHostState;

typedef struct {
    char          name[XBOX_HOST_NAME_MAX];
    char          ip[XBOX_HOST_IP_MAX];
    char          id[XBOX_HOST_ID_MAX];
    XboxHostState state;
    int           is_manual;
} XboxHost;

typedef struct {
    XboxHost hosts[XBOX_MAX_HOSTS];
    int      count;
} XboxHostList;

XboxHostList* xbox_host_list_create(void);
void          xbox_host_list_destroy(XboxHostList* list);

/* Fetch consoles registered to the Xbox Live account via xccs.xboxlive.com.
   Requires the XSTS token (http://xboxlive.com relying party) and uhs from auth.
   Returns number of consoles found, or -1 on error. */
int  xbox_host_fetch_from_api(XboxHostList* list,
                              const char* uhs,
                              const char* xsts_token);

/* Scan local network for Xbox consoles via mDNS (_xbox-smartglass._tcp.local).
   Returns number of new hosts found, or -1 on error.
   TODO: implement real mDNS using switch-mdns / lwIP multicast. */
int  xbox_host_discover(XboxHostList* list);

/* Add a console by IP address (entered manually by the user). */
int  xbox_host_add_manual(XboxHostList* list, const char* name, const char* ip);

void xbox_host_list_clear(XboxHostList* list);

const char* xbox_host_state_string(XboxHostState state);
const char* xbox_host_state_string_en(XboxHostState state);

#ifdef __cplusplus
}
#endif

#endif // XBOX_HOST_H

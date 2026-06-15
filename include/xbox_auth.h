#ifndef XBOX_AUTH_H
#define XBOX_AUTH_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Constants                                                            */
/* ------------------------------------------------------------------ */

#define XBOX_AUTH_CLIENT_ID     "1f907974-e22b-4810-a9de-d9647380c97e"
#define XBOX_AUTH_SCOPE         "XboxLive.signin offline_access"
#define XBOX_AUTH_TOKEN_FILE    "/switch/greenlight/auth.json"

/* ------------------------------------------------------------------ */
/* Types                                                                */
/* ------------------------------------------------------------------ */

typedef enum {
    XBOX_AUTH_STATE_NONE = 0,
    XBOX_AUTH_STATE_WAITING,    /* device code shown, waiting for user */
    XBOX_AUTH_STATE_POLLING,    /* polling Microsoft for token */
    XBOX_AUTH_STATE_OK,         /* fully authenticated */
    XBOX_AUTH_STATE_EXPIRED,    /* XSTS token expired, needs refresh */
    XBOX_AUTH_STATE_ERROR,
} XboxAuthState;

typedef struct {
    /* --- Device code flow --- */
    char user_code[16];         /* shown to user: "ABCD-1234"         */
    char device_code[1024];     /* internal code sent to Microsoft     */
    char verification_uri[64];  /* typically "https://microsoft.com/link" */
    int  expires_in;            /* seconds until device_code expires   */
    int  poll_interval;         /* seconds between token poll requests */

    /* --- Tokens --- */
    char* access_token;         /* MSA access token (heap-allocated)   */
    char* refresh_token;        /* MSA refresh token (heap-allocated)  */
    char  xbl_token[4096];      /* Xbox Live token                     */
    char  xsts_token[4096];     /* XSTS token (http://xboxlive.com)    */
    char  gssv_token[4096];     /* XSTS token (http://gssv.xboxlive.com) */
    char  gs_token[4096];       /* gsToken for streaming API (Bearer)  */
    char  streaming_host[256];  /* host from offeringSettings.regions  */
    char  uhs[64];              /* Xbox user hash                      */
    char  gamertag[64];         /* display name (optional)             */

    /* --- State --- */
    XboxAuthState state;
    time_t        access_expiry;    /* UTC epoch when access_token expires */
    time_t        xsts_expiry;      /* UTC epoch when xsts_token expires   */
    char          error[256];

} XboxAuthContext;

/* ------------------------------------------------------------------ */
/* Lifecycle                                                            */
/* ------------------------------------------------------------------ */

XboxAuthContext* xbox_auth_create(void);
void             xbox_auth_destroy(XboxAuthContext* ctx);

/* ------------------------------------------------------------------ */
/* Device code flow                                                     */
/* ------------------------------------------------------------------ */

/* Step 1 — request a device code from Microsoft.
   On success: ctx->user_code and ctx->verification_uri are filled,
   ctx->state becomes XBOX_AUTH_STATE_WAITING.
   Returns 0 on success, -1 on error. */
int xbox_auth_request_device_code(XboxAuthContext* ctx);

/* Step 2 — poll once for the access token (call every poll_interval seconds).
   Returns:  1 = authenticated (proceed to exchange),
             0 = still pending (keep polling),
            -1 = error / expired. */
int xbox_auth_poll_token(XboxAuthContext* ctx);

/* Step 3 — exchange MSA access_token for XBL then XSTS (http://xboxlive.com).
   Fills xbl_token, xsts_token, uhs.
   Returns 0 on success, -1 on error. */
int xbox_auth_exchange_tokens(XboxAuthContext* ctx);

/* Step 4 — get GSSV XSTS then xhome streaming token.
   Fills gssv_token, gs_token, streaming_host.
   Must be called after xbox_auth_exchange_tokens().
   Returns 0 on success, -1 on error. */
int xbox_auth_get_streaming_tokens(XboxAuthContext* ctx);

/* ------------------------------------------------------------------ */
/* Token refresh                                                        */
/* ------------------------------------------------------------------ */

/* Refresh MSA token using refresh_token, then re-exchange for XBL/XSTS.
   Returns 0 on success, -1 on error (user must re-authenticate). */
int xbox_auth_refresh(XboxAuthContext* ctx);

/* ------------------------------------------------------------------ */
/* Persistence                                                          */
/* ------------------------------------------------------------------ */

/* Save tokens to path (e.g. XBOX_AUTH_TOKEN_FILE).
   Creates parent directories if missing.
   Returns 0 on success, -1 on error. */
int xbox_auth_save(const XboxAuthContext* ctx, const char* path);

/* Load tokens from path.
   Returns 0 on success, -1 if file not found or invalid. */
int xbox_auth_load(XboxAuthContext* ctx, const char* path);

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

/* Returns 1 if the XSTS token is still valid (not expired). */
int xbox_auth_is_valid(const XboxAuthContext* ctx);

/* Fill out with "XBL3.0 x=<uhs>;<xsts_token>" (for Authorization header). */
void xbox_auth_get_header(const XboxAuthContext* ctx, char* out, int out_size);

const char* xbox_auth_state_string(XboxAuthState state);

#ifdef __cplusplus
}
#endif

#endif // XBOX_AUTH_H

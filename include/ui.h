#ifndef XCLOUD_UI_H
#define XCLOUD_UI_H

#include "xbox_host.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_SCREEN_HOST_LIST,    /* main screen — discovered Xbox consoles */
    UI_SCREEN_LOGIN,        /* authenticate with Xbox Live */
    UI_SCREEN_CONNECTING,   /* connecting to selected console */
    UI_SCREEN_STREAMING,    /* active stream (future) */
    UI_SCREEN_PAUSED,       /* stream paused */
    UI_SCREEN_ERROR,        /* error display */
} UIScreen;

/* Core lifecycle */
int  ui_init(void);
void ui_deinit(void);

/* Screen rendering */
void ui_draw_screen(UIScreen screen);

/* State setters — call before ui_draw_screen to pass data to the renderer */
void ui_set_host_list(const XboxHostList* list, int selected_index);
void ui_set_connecting_host(const XboxHost* host, const char* status);
void ui_set_error(const char* message);
void ui_set_login_state(const char* user_code, const char* status, int expires_in);

/* Debug log overlay — shown on screen during development.
   Behaves like printf; last 8 lines are drawn above the footer. */
void ui_log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/* Low-level helpers (used by console fallback) */
void ui_draw_text(int x, int y, const char* text);
void ui_clear_screen(void);
void ui_flip_buffer(void);

#ifdef __cplusplus
}
#endif

#endif // XCLOUD_UI_H

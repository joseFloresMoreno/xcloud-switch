#ifndef XCLOUD_UI_H
#define XCLOUD_UI_H

typedef enum {
    UI_SCREEN_MAIN_MENU,
    UI_SCREEN_LOGIN,
    UI_SCREEN_GAME_LIST,
    UI_SCREEN_CONNECTING,
    UI_SCREEN_STREAMING,
    UI_SCREEN_PAUSED,
    UI_SCREEN_ERROR,
} UIScreen;

// UI functions
int ui_init(void);
void ui_deinit(void);
void ui_draw_screen(UIScreen screen);
void ui_draw_text(int x, int y, const char* text);
void ui_draw_loading_spinner(int x, int y);
void ui_draw_error(const char* error_message);
void ui_clear_screen(void);
void ui_flip_buffer(void);

#endif // XCLOUD_UI_H

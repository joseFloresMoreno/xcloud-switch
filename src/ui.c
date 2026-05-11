#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

int ui_init(void) {
    printf("[UI] UI initialized\n");
    return 0;
}

void ui_deinit(void) {
    printf("[UI] UI deinitialized\n");
}

void ui_clear_screen(void) {
    printf("\x1b[2J\x1b[H");  // ANSI clear screen
}

void ui_draw_text(int x, int y, const char* text) {
    if (!text) return;
    printf("\x1b[%d;%dH%s", y, x, text);
}

void ui_draw_loading_spinner(int x, int y) {
    static int spinner_state = 0;
    const char* spinner[] = {"|", "/", "-", "\\"};
    printf("\x1b[%d;%dH%s", y, x, spinner[spinner_state % 4]);
    spinner_state++;
}

void ui_draw_error(const char* error_message) {
    if (!error_message) return;
    printf("\x1b[31mERROR: %s\x1b[0m\n", error_message);
}

void ui_flip_buffer(void) {
    fflush(stdout);
}

void ui_draw_screen(UIScreen screen) {
    switch (screen) {
        case UI_SCREEN_MAIN_MENU:
            printf("╔════════════════════════════════════════╗\n");
            printf("║  Xbox xCloud Client for Nintendo Switch║\n");
            printf("║           Version 0.1 (Alpha)          ║\n");
            printf("╚════════════════════════════════════════╝\n");
            printf("\nMain Menu:\n");
            printf("  [A] - Play Game\n");
            printf("  [B] - Exit\n\n");
            break;
        case UI_SCREEN_LOGIN:
            printf("Login Screen:\n");
            printf("  Enter your Microsoft account token:\n");
            break;
        case UI_SCREEN_GAME_LIST:
            printf("Game List:\n");
            printf("  Loading games...\n");
            break;
        case UI_SCREEN_CONNECTING:
            printf("Connecting...\n");
            ui_draw_loading_spinner(0, 0);
            break;
        case UI_SCREEN_STREAMING:
            printf("Streaming game...\n");
            break;
        case UI_SCREEN_PAUSED:
            printf("Game Paused\n");
            break;
        case UI_SCREEN_ERROR:
            printf("Error occurred\n");
            break;
        default:
            printf("Unknown screen: %d\n", screen);
    }
}

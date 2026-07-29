/*
 * SDL2 UI — Greenlight Switch
 *
 * Inspired by Moonlight-Switch's visual style:
 *   - Dark background with Xbox-green accent
 *   - Card-based host list (1 per row, full width)
 *   - Switch-style header / footer bars
 *   - System font via plGetSharedFontByType (no TTF files needed)
 *
 * This file is compiled in place of ui.c when HAVE_SDL2 is defined.
 * The public API is the same C interface declared in include/ui.h.
 */

#include "ui.h"
#include "xbox_auth.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <switch.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#define UI_LOG_FILE "/switch/greenlight/log.txt"

/* ------------------------------------------------------------------ */
/* Layout constants (1280 x 720)                                        */
/* ------------------------------------------------------------------ */

static const int SW = 1280;
static const int SH = 720;

static const int HEADER_H  = 72;
static const int FOOTER_H  = 60;
static const int CONTENT_Y = 72;
static const int CONTENT_H = 720 - 72 - 60;   /* 588 */

/* Card geometry */
static const int CARD_X    = 40;
static const int CARD_W    = SW - 80;
static const int CARD_H    = 110;
static const int CARD_GAP  = 12;

/* ------------------------------------------------------------------ */
/* Color palette (Xbox dark theme)                                      */
/* ------------------------------------------------------------------ */

static SDL_Color C_BG           = {  11,  17,  23, 255 };
static SDL_Color C_HEADER       = {  16,  28,  16, 255 };   /* dark Xbox green */
static SDL_Color C_FOOTER       = {  11,  17,  23, 220 };
static SDL_Color C_CARD         = {  22,  27,  34, 255 };
static SDL_Color C_CARD_SEL     = {  16, 100,  16, 255 };   /* selected — Xbox green */
static SDL_Color C_CARD_BORDER  = {  52, 179,  80, 255 };   /* accent green */
static SDL_Color C_TEXT         = { 201, 209, 217, 255 };
static SDL_Color C_TEXT_DIM     = { 139, 148, 158, 255 };
static SDL_Color C_TEXT_SEL     = { 255, 255, 255, 255 };
static SDL_Color C_GREEN        = {  63, 185,  80, 255 };
static SDL_Color C_RED          = { 248,  81,  73, 255 };
static SDL_Color C_YELLOW       = { 210, 153,  34, 255 };
static SDL_Color C_ACCENT       = {  52, 179,  80, 255 };   /* Xbox green */

/* ------------------------------------------------------------------ */
/* State                                                                */
/* ------------------------------------------------------------------ */

static SDL_Window*   g_window   = NULL;
static SDL_Renderer* g_renderer = NULL;
static TTF_Font*     g_font_lg  = NULL;   /* 32px — titles */
static TTF_Font*     g_font_md  = NULL;   /* 22px — body */
static TTF_Font*     g_font_sm  = NULL;   /* 17px — hints / dims */

static SDL_RWops* g_font_rw = NULL;       /* keeps RWops alive for font lifetime */

static const XboxHostList* g_hosts   = NULL;
static int                 g_sel     = 0;
static const XboxHost*     g_conn    = NULL;
static char                g_conn_status[128] = {0};
static char                g_error[512]       = {0};

static int g_spinner_tick = 0;

/* Login screen state */
static char g_login_user_code[16]  = {0};
static char g_login_url[64]        = "microsoft.com/link";
static char g_login_status[128]    = {0};
static int  g_login_expires_in     = 0;

/* Debug log overlay */
#define UI_LOG_LINES 12
#define UI_LOG_W     160
static char g_log_buf[UI_LOG_LINES][UI_LOG_W] = {};
static int  g_log_head = 0;   /* index of next write slot (mod UI_LOG_LINES) */

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

static void set_color(SDL_Color c) {
    SDL_SetRenderDrawColor(g_renderer, c.r, c.g, c.b, c.a);
}

static void fill_rect(int x, int y, int w, int h, SDL_Color c) {
    set_color(c);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(g_renderer, &r);
}

static void draw_rect_outline(int x, int y, int w, int h, SDL_Color c, int thickness) {
    set_color(c);
    for (int i = 0; i < thickness; i++) {
        SDL_Rect r = {x + i, y + i, w - i * 2, h - i * 2};
        SDL_RenderDrawRect(g_renderer, &r);
    }
}

static void draw_text(TTF_Font* font, const char* text, int x, int y, SDL_Color c) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, c);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    if (tex) {
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(g_renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

static void draw_text_centered(TTF_Font* font, const char* text, int cx, int y, SDL_Color c) {
    if (!font || !text || text[0] == '\0') return;
    int tw, th;
    TTF_SizeUTF8(font, text, &tw, &th);
    draw_text(font, text, cx - tw / 2, y, c);
}

/* Vertical center within a rect */
static void draw_text_vcenter(TTF_Font* font, const char* text,
                               int x, int rect_y, int rect_h, SDL_Color c) {
    if (!font || !text || text[0] == '\0') return;
    int tw, th;
    TTF_SizeUTF8(font, text, &tw, &th);
    draw_text(font, text, x, rect_y + (rect_h - th) / 2, c);
}

/* Spinning dots indicator (frame-based) */
static void draw_spinner(int cx, int cy, int radius) {
    const int N     = 8;
    const int tick  = g_spinner_tick / 4;   /* slow it down */
    for (int i = 0; i < N; i++) {
        double angle = (2.0 * 3.14159265 / N) * i;
        int dx = (int)(radius * SDL_sin(angle));
        int dy = (int)(radius * -SDL_cos(angle));
        int alpha = 60 + (int)(195.0 * ((i + tick) % N) / (N - 1));
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(g_renderer, C_ACCENT.r, C_ACCENT.g, C_ACCENT.b, (Uint8)alpha);
        SDL_Rect dot = {cx + dx - 5, cy + dy - 5, 10, 10};
        SDL_RenderFillRect(g_renderer, &dot);
    }
}

/* ------------------------------------------------------------------ */
/* Structural chrome                                                    */
/* ------------------------------------------------------------------ */

static void draw_header(const char* title) {
    fill_rect(0, 0, SW, HEADER_H, C_HEADER);

    /* left: Xbox green square logo placeholder */
    fill_rect(16, 16, 40, 40, C_ACCENT);
    draw_text_vcenter(g_font_lg, "G", 24, 0, HEADER_H, C_BG);

    /* center: screen title */
    draw_text_vcenter(g_font_lg, title, 68, 0, HEADER_H, C_TEXT_SEL);

    /* right: version */
    draw_text_vcenter(g_font_sm, "v0.1 alpha", SW - 120, 0, HEADER_H, C_TEXT_DIM);
}

static void draw_footer(const char* hints) {
    fill_rect(0, SH - FOOTER_H, SW, FOOTER_H, C_FOOTER);
    /* separator line */
    fill_rect(0, SH - FOOTER_H, SW, 1, C_CARD);
    if (hints)
        draw_text_vcenter(g_font_sm, hints, 30, SH - FOOTER_H, FOOTER_H, C_TEXT_DIM);
}

/* ------------------------------------------------------------------ */
/* HOST LIST screen                                                     */
/* ------------------------------------------------------------------ */

static SDL_Color state_color(XboxHostState s) {
    switch (s) {
        case XBOX_HOST_STATE_AVAILABLE: return C_GREEN;
        case XBOX_HOST_STATE_BUSY:      return C_YELLOW;
        case XBOX_HOST_STATE_OFFLINE:   return C_RED;
        default:                        return C_TEXT_DIM;
    }
}

static void draw_host_card(const XboxHost* h, int card_y, bool selected) {
    SDL_Color bg = selected ? C_CARD_SEL : C_CARD;
    fill_rect(CARD_X, card_y, CARD_W, CARD_H, bg);

    if (selected)
        draw_rect_outline(CARD_X, card_y, CARD_W, CARD_H, C_CARD_BORDER, 2);

    /* Console icon rectangle */
    fill_rect(CARD_X + 16, card_y + (CARD_H - 56) / 2, 72, 56, C_HEADER);
    draw_text_centered(g_font_md, "XBOX",
                       CARD_X + 16 + 36,
                       card_y + (CARD_H - 56) / 2 + 16,
                       C_ACCENT);

    /* Name */
    SDL_Color name_c = selected ? C_TEXT_SEL : C_TEXT;
    draw_text(g_font_md, h->name,
              CARD_X + 108, card_y + 18, name_c);

    /* IP */
    draw_text(g_font_sm, h->ip,
              CARD_X + 108, card_y + 46, C_TEXT_DIM);

    /* Status dot + text */
    SDL_Color sc = state_color(h->state);
    fill_rect(CARD_X + 108, card_y + 74, 10, 10, sc);
    draw_text(g_font_sm, xbox_host_state_string(h->state),
              CARD_X + 124, card_y + 72, sc);

    if (h->is_manual) {
        draw_text(g_font_sm, "(manual)",
                  CARD_X + CARD_W - 100, card_y + 72, C_TEXT_DIM);
    }
}

static void draw_no_hosts(void) {
    int cy = CONTENT_Y + CONTENT_H / 2 - 40;
    draw_text_centered(g_font_md, "No se encontraron consolas Xbox.", SW / 2, cy, C_TEXT_DIM);
    draw_text_centered(g_font_sm, "Asegurate de estar en la misma red Wi-Fi que tu Xbox.",
                       SW / 2, cy + 34, C_TEXT_DIM);
    draw_text_centered(g_font_sm, "Presiona [Y] para agregar una consola por IP.",
                       SW / 2, cy + 58, C_TEXT_DIM);
}

static void draw_host_list_screen(void) {
    fill_rect(0, 0, SW, SH, C_BG);
    draw_header("Xbox Local Streaming");

    if (!g_hosts || g_hosts->count == 0) {
        draw_no_hosts();
    } else {
        int y = CONTENT_Y + 16;
        for (int i = 0; i < g_hosts->count && y + CARD_H <= SH - FOOTER_H; i++) {
            draw_host_card(&g_hosts->hosts[i], y, i == g_sel);
            y += CARD_H + CARD_GAP;
        }
    }

    draw_footer("[A] Conectar    [X] Refrescar    [+] Salir");
}

/* ------------------------------------------------------------------ */
/* CONNECTING screen                                                    */
/* ------------------------------------------------------------------ */

static void draw_connecting_screen(void) {
    fill_rect(0, 0, SW, SH, C_BG);
    draw_header("Conectando...");

    int cy = SH / 2 - 40;
    draw_spinner(SW / 2, cy, 40);
    g_spinner_tick++;

    if (g_conn) {
        char line[128];
        snprintf(line, sizeof(line), "Conectando a: %s (%s)", g_conn->name, g_conn->ip);
        draw_text_centered(g_font_md, line, SW / 2, cy + 60, C_TEXT);
    }

    if (g_conn_status[0])
        draw_text_centered(g_font_sm, g_conn_status, SW / 2, cy + 96, C_TEXT_DIM);

    draw_footer("[B] Cancelar");
}

/* ------------------------------------------------------------------ */
/* ERROR screen                                                         */
/* ------------------------------------------------------------------ */

static void draw_error_screen(void) {
    fill_rect(0, 0, SW, SH, C_BG);
    draw_header("Error");

    const char* msg = g_error[0] ? g_error : "Ha ocurrido un error desconocido.";
    int tw, th;
    TTF_SizeUTF8(g_font_md, msg, &tw, &th);

    /* Red error box */
    int box_y = SH / 2 - 50;
    fill_rect(CARD_X, box_y, CARD_W, 100, C_CARD);
    draw_rect_outline(CARD_X, box_y, CARD_W, 100, C_RED, 2);

    fill_rect(CARD_X + 16, box_y + 20, 8, 60, C_RED);   /* red side bar */

    draw_text(g_font_md, msg, CARD_X + 36, box_y + (100 - th) / 2, C_TEXT);

    draw_footer("[B] Volver");
}

/* ------------------------------------------------------------------ */
/* LOGIN screen — device code flow                                      */
/* ------------------------------------------------------------------ */

static void draw_login_screen(void) {
    fill_rect(0, 0, SW, SH, C_BG);
    draw_header("Iniciar sesion con Xbox Live");

    int cx = SW / 2;
    int base_y = CONTENT_Y + 30;

    /* Instruction line */
    draw_text_centered(g_font_md,
        "Para conectar tu cuenta Xbox, visita en tu telefono o PC:",
        cx, base_y, C_TEXT_DIM);

    /* URL box */
    int url_box_y = base_y + 44;
    fill_rect(cx - 220, url_box_y, 440, 56, C_CARD);
    draw_rect_outline(cx - 220, url_box_y, 440, 56, C_ACCENT, 2);
    draw_text_centered(g_font_lg, "microsoft.com/link", cx, url_box_y + 10, C_ACCENT);

    /* "Enter this code" label */
    draw_text_centered(g_font_md, "e ingresa el codigo:", cx, url_box_y + 72, C_TEXT_DIM);

    /* Big user code box */
    if (g_login_user_code[0]) {
        int code_box_y = url_box_y + 110;
        fill_rect(cx - 160, code_box_y, 320, 90, C_CARD_SEL);
        draw_rect_outline(cx - 160, code_box_y, 320, 90, C_CARD_BORDER, 3);

        /* Extra-large font for the code — use lg font but render twice as visible */
        TTF_Font* font_code = g_font_lg;  /* 32px — readable on TV */
        if (font_code) {
            int tw, th;
            TTF_SizeUTF8(font_code, g_login_user_code, &tw, &th);
            /* Draw a subtle shadow for readability */
            draw_text(font_code, g_login_user_code,
                      cx - tw / 2 + 2, code_box_y + (90 - th) / 2 + 2,
                      (SDL_Color){0, 0, 0, 180});
            draw_text(font_code, g_login_user_code,
                      cx - tw / 2, code_box_y + (90 - th) / 2,
                      C_TEXT_SEL);
        }

        /* Countdown / status */
        if (g_login_status[0]) {
            draw_text_centered(g_font_sm, g_login_status,
                               cx, code_box_y + 106, C_TEXT_DIM);
        }

        /* Spinner while polling */
        draw_spinner(cx, code_box_y + 150, 20);
        g_spinner_tick++;
    } else {
        /* No code yet — show loading */
        draw_spinner(cx, url_box_y + 160, 24);
        g_spinner_tick++;
        draw_text_centered(g_font_sm, "Solicitando codigo...", cx, url_box_y + 200, C_TEXT_DIM);
    }

    draw_footer("[B] Cancelar    [+] Salir");
}

/* ------------------------------------------------------------------ */
/* Public C API (extern "C" linkage comes from the header guards)       */
/* ------------------------------------------------------------------ */

int ui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "[UI] SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "[UI] TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    g_window = SDL_CreateWindow("Greenlight",
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                SW, SH,
                                SDL_WINDOW_FULLSCREEN);
    if (!g_window) {
        fprintf(stderr, "[UI] CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return -1;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1,
                                    SDL_RENDERER_ACCELERATED |
                                    SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        fprintf(stderr, "[UI] CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        TTF_Quit(); SDL_Quit();
        return -1;
    }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    /* Load Switch system font — no TTF files needed */
    if (plInitialize(PlServiceType_User) == 0) {
        PlFontData fd;
        if (plGetSharedFontByType(&fd, PlSharedFontType_Standard) == 0 && fd.size > 0) {
            g_font_rw = SDL_RWFromMem(fd.address, (int)fd.size);
            if (g_font_rw) {
                /* Open the same RWops three times at different sizes.
                   SDL_RWFromMem is seekable so we seek to 0 before each open. */
                SDL_RWseek(g_font_rw, 0, RW_SEEK_SET);
                g_font_lg = TTF_OpenFontRW(g_font_rw, 0, 32);
                SDL_RWseek(g_font_rw, 0, RW_SEEK_SET);
                g_font_md = TTF_OpenFontRW(g_font_rw, 0, 22);
                SDL_RWseek(g_font_rw, 0, RW_SEEK_SET);
                g_font_sm = TTF_OpenFontRW(g_font_rw, 0, 17);
            }
        }
    }

    if (!g_font_lg || !g_font_md || !g_font_sm)
        fprintf(stderr, "[UI] Warning: could not load system font; text will be absent.\n");

    printf("[UI] SDL2 initialized (%dx%d)\n", SW, SH);
    return 0;
}

void ui_deinit(void) {
    if (g_font_lg) { TTF_CloseFont(g_font_lg); g_font_lg = NULL; }
    if (g_font_md) { TTF_CloseFont(g_font_md); g_font_md = NULL; }
    if (g_font_sm) { TTF_CloseFont(g_font_sm); g_font_sm = NULL; }
    if (g_font_rw) { SDL_RWclose(g_font_rw);   g_font_rw = NULL; }
    plExit();
    if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = NULL; }
    if (g_window)   { SDL_DestroyWindow(g_window);     g_window   = NULL; }
    TTF_Quit();
    SDL_Quit();
}

void ui_set_host_list(const XboxHostList* list, int selected_index) {
    g_hosts = list;
    g_sel   = selected_index;
}

void ui_set_connecting_host(const XboxHost* host, const char* status) {
    g_conn = host;
    if (status)
        snprintf(g_conn_status, sizeof(g_conn_status), "%s", status);
    else
        g_conn_status[0] = '\0';
}

void ui_set_error(const char* message) {
    if (message)
        snprintf(g_error, sizeof(g_error), "%s", message);
    else
        g_error[0] = '\0';
}

void ui_set_login_state(const char* user_code, const char* status, int expires_in) {
    if (user_code)
        snprintf(g_login_user_code, sizeof(g_login_user_code), "%s", user_code);
    else
        g_login_user_code[0] = '\0';

    if (status)
        snprintf(g_login_status, sizeof(g_login_status), "%s", status);
    else
        g_login_status[0] = '\0';

    g_login_expires_in = expires_in;
}

void ui_draw_screen(UIScreen screen) {
    if (!g_renderer) return;

    SDL_SetRenderDrawColor(g_renderer, C_BG.r, C_BG.g, C_BG.b, 255);
    SDL_RenderClear(g_renderer);

    switch (screen) {
        case UI_SCREEN_HOST_LIST:  draw_host_list_screen();  break;
        case UI_SCREEN_CONNECTING: draw_connecting_screen(); break;
        case UI_SCREEN_ERROR:      draw_error_screen();      break;

        case UI_SCREEN_LOGIN:   draw_login_screen(); break;

        case UI_SCREEN_STREAMING:
            fill_rect(0, 0, SW, SH, C_BG);
            draw_text_centered(g_font_lg, "Transmitiendo...", SW / 2, SH / 2 - 16, C_TEXT_DIM);
            break;

        case UI_SCREEN_PAUSED:
            fill_rect(0, 0, SW, SH, C_BG);
            draw_header("Pausado");
            draw_text_centered(g_font_md, "Stream pausado.", SW / 2, SH / 2 - 20, C_TEXT);
            draw_footer("[A] Reanudar    [B] Salir");
            break;

        default:
            break;
    }

    /* Debug log overlay — drawn on every screen above the footer */
    if (g_log_head > 0 && g_font_sm) {
        const int LINE_H  = 19;
        const int PAD     = 6;
        const int LINES   = UI_LOG_LINES;
        const int OVH     = LINES * LINE_H + PAD * 2;
        const int OVY     = SH - FOOTER_H - OVH;

        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 200);
        SDL_Rect ov = {0, OVY, SW, OVH};
        SDL_RenderFillRect(g_renderer, &ov);
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

        /* Print oldest→newest from ring buffer */
        SDL_Color dim = {120, 220, 120, 255};
        int total = g_log_head < LINES ? g_log_head : LINES;
        for (int i = 0; i < total; i++) {
            int slot = (g_log_head - total + i) % LINES;
            if (slot < 0) slot += LINES;
            int ly = OVY + PAD + i * LINE_H;
            draw_text(g_font_sm, g_log_buf[slot], 8, ly, dim);
        }
    }

    SDL_RenderPresent(g_renderer);
}

void ui_clear_screen(void) {
    if (!g_renderer) return;
    set_color(C_BG);
    SDL_RenderClear(g_renderer);
}

void ui_flip_buffer(void) {
    if (g_renderer)
        SDL_RenderPresent(g_renderer);
}

void ui_draw_text(int x, int y, const char* text) {
    draw_text(g_font_md, text, x, y, C_TEXT);
}

void ui_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int slot = g_log_head % UI_LOG_LINES;
    vsnprintf(g_log_buf[slot], UI_LOG_W, fmt, ap);
    va_end(ap);
    g_log_head++;
    /* Mirror to stdout for nxlink */
    printf("[UI] %s\n", g_log_buf[slot]);

    /* Mirror to a plain text file on the SD card so logs can be pulled off
       hardware without nxlink — opened/closed per line (low volume, app may
       crash) so nothing is lost if the process dies mid-session. */
    static bool log_dir_ready = false;
    if (!log_dir_ready) {
        mkdir("/switch", 0777);
        mkdir("/switch/greenlight", 0777);
        log_dir_ready = true;
    }
    FILE* f = fopen(UI_LOG_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "[%02d:%02d:%02d] %s\n",
                t->tm_hour, t->tm_min, t->tm_sec, g_log_buf[slot]);
        fclose(f);
    }
}

void ui_log_dump(const char* label, const char* text) {
    printf("[UI] %s: %s\n", label, text ? text : "(null)");
    FILE* f = fopen(UI_LOG_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "[%02d:%02d:%02d] ---- %s ----\n%s\n---- end %s ----\n",
                t->tm_hour, t->tm_min, t->tm_sec, label, text ? text : "(null)", label);
        fclose(f);
    }
}

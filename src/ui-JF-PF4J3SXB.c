/* Console-mode UI fallback — used when HAVE_SDL2 is not defined.
   All rendering goes through libnx printf/ANSI to the Switch console. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include "ui.h"

#define UI_LOG_FILE "/switch/greenlight/log.txt"

static const XboxHostList* g_hosts   = NULL;
static int                 g_sel     = 0;
static const XboxHost*     g_conn    = NULL;
static char                g_conn_status[128] = {0};
static char                g_error[512]       = {0};

static int g_spinner = 0;
static const char* SPINNER[] = {"|", "/", "-", "\\"};

/* ------------------------------------------------------------------ */

int ui_init(void) {
    printf("[UI] console mode\n");
    return 0;
}

void ui_deinit(void) {}

void ui_clear_screen(void) {
    printf("\x1b[2J\x1b[H");
}

void ui_draw_text(int x, int y, const char* text) {
    if (!text) return;
    printf("\x1b[%d;%dH%s", y, x, text);
}

void ui_flip_buffer(void) {
    fflush(stdout);
}

/* ------------------------------------------------------------------ */

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

void ui_set_login_state(const char* user_code, const char* status, int expires_in) {
    (void)expires_in;
    if (user_code && user_code[0])
        printf("[LOGIN] Visita microsoft.com/link e ingresa: %s\n", user_code);
    if (status && status[0])
        printf("[LOGIN] %s\n", status);
}

void ui_set_error(const char* message) {
    if (message)
        snprintf(g_error, sizeof(g_error), "%s", message);
    else
        g_error[0] = '\0';
}

/* ------------------------------------------------------------------ */

static void draw_header(void) {
    printf("===== Greenlight — Xbox Local Streaming =====\n\n");
}

static void draw_host_list(void) {
    draw_header();
    printf("Consolas Xbox en la red local:\n\n");

    if (!g_hosts || g_hosts->count == 0) {
        printf("  No se encontraron consolas.\n");
        printf("  Asegurate de estar en la misma red que tu Xbox.\n\n");
        printf("  [Y] Agregar IP manual   [+] Salir\n");
        return;
    }

    for (int i = 0; i < g_hosts->count; i++) {
        const XboxHost* h = &g_hosts->hosts[i];
        const char* cursor = (i == g_sel) ? ">> " : "   ";
        printf("%s[%d] %s\n", cursor, i + 1, h->name);
        printf("       IP: %s\n", h->ip);
        printf("       Estado: %s\n\n", xbox_host_state_string(h->state));
    }

    printf("[A] Conectar   [Y] Agregar manual   [+] Salir\n");
}

static void draw_connecting(void) {
    draw_header();
    printf("Conectando... %s\n\n", SPINNER[g_spinner % 4]);
    g_spinner++;

    if (g_conn) {
        printf("  Consola : %s\n", g_conn->name);
        printf("  IP      : %s\n", g_conn->ip);
    }
    if (g_conn_status[0])
        printf("  Estado  : %s\n", g_conn_status);

    printf("\n  [B] Cancelar\n");
}

static void draw_error(void) {
    draw_header();
    printf("\x1b[31m[ERROR]\x1b[0m %s\n\n", g_error[0] ? g_error : "Error desconocido.");
    printf("  [B] Volver\n");
}

void ui_draw_screen(UIScreen screen) {
    switch (screen) {
        case UI_SCREEN_HOST_LIST:   draw_host_list();   break;
        case UI_SCREEN_CONNECTING:  draw_connecting();  break;
        case UI_SCREEN_ERROR:       draw_error();       break;
        case UI_SCREEN_LOGIN:
            draw_header();
            printf("Inicio de sesion Xbox Live\n");
            printf("  Ingresa tu token de autenticacion.\n");
            break;
        case UI_SCREEN_STREAMING:
            draw_header();
            printf("Transmitiendo... (pendiente)\n");
            break;
        case UI_SCREEN_PAUSED:
            draw_header();
            printf("Stream pausado. [A] Reanudar  [B] Salir\n");
            break;
        default:
            printf("Pantalla desconocida: %d\n", screen);
            break;
    }
}

void ui_log(const char* fmt, ...) {
    char line[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    printf("%s\n", line);

    static int log_dir_ready = 0;
    if (!log_dir_ready) {
        mkdir("/switch", 0777);
        mkdir("/switch/greenlight", 0777);
        log_dir_ready = 1;
    }
    FILE* f = fopen(UI_LOG_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "[%02d:%02d:%02d] %s\n", t->tm_hour, t->tm_min, t->tm_sec, line);
        fclose(f);
    }
}

void ui_log_dump(const char* label, const char* text) {
    printf("%s: %s\n", label, text ? text : "(null)");
    FILE* f = fopen(UI_LOG_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        fprintf(f, "[%02d:%02d:%02d] ---- %s ----\n%s\n---- end %s ----\n",
                t->tm_hour, t->tm_min, t->tm_sec, label, text ? text : "(null)", label);
        fclose(f);
    }
}

#include <stdio.h>
#include <stdlib.h>

#include "session.h"
#include "xcloud_api.h"
#include "webrtc_client.h"
#include "video_decoder.h"
#include "audio_decoder.h"
#include "input_handler.h"
#include "ui.h"

// Application state
typedef struct {
    int running;
    UIScreen current_screen;
    XCloudSession* active_session;
    XCloudApiClient* api_client;
    XCloudWebRTCClient* webrtc_client;
    XCloudVideoDecoder* video_decoder;
    XCloudAudioDecoder* audio_decoder;
} AppState;

static AppState g_app_state = {0};

// Forward declarations
static void app_init(void);
static void app_deinit(void);
static void app_main_loop(void);
static void app_handle_input(void);
static void app_render(void);

int main(int argc, char* argv[]) {
    printf("=== Xbox xCloud Client for Switch ===\n");
    printf("Starting initialization...\n\n");
    
    // Initialize application
    app_init();
    
    // Main loop
    app_main_loop();
    
    // Cleanup
    app_deinit();
    
    printf("Application terminated.\n");
    return 0;
}

static void app_init(void) {
    printf("[INIT] Initializing application...\n");
    
    g_app_state.running = 1;
    g_app_state.current_screen = UI_SCREEN_MAIN_MENU;
    
    // Initialize UI
    printf("[INIT] Initializing UI...\n");
    if (ui_init() < 0) {
        printf("[ERROR] Failed to initialize UI\n");
        return;
    }
    
    // Initialize input handler
    printf("[INIT] Initializing input handler...\n");
    if (input_handler_init() < 0) {
        printf("[ERROR] Failed to initialize input handler\n");
        return;
    }
    
    printf("[INIT] Application ready!\n\n");
}

static void app_deinit(void) {
    printf("[DEINIT] Cleaning up application...\n");
    
    if (g_app_state.active_session) {
        session_destroy(g_app_state.active_session);
    }
    
    if (g_app_state.api_client) {
        xcloud_api_destroy(g_app_state.api_client);
    }
    
    if (g_app_state.webrtc_client) {
        webrtc_destroy(g_app_state.webrtc_client);
    }
    
    if (g_app_state.video_decoder) {
        video_decoder_destroy(g_app_state.video_decoder);
    }
    
    if (g_app_state.audio_decoder) {
        audio_decoder_destroy(g_app_state.audio_decoder);
    }
    
    input_handler_deinit();
    ui_deinit();
    
    printf("[DEINIT] Cleanup complete\n");
}

static void app_main_loop(void) {
    printf("[LOOP] Entering main loop...\n\n");
    
    int frame_count = 0;
    int max_frames = 300;  // 5 seconds at 60 FPS for test
    
    while (frame_count < max_frames && g_app_state.running) {
        // Handle input
        app_handle_input();
        
        // Render
        app_render();
        
        frame_count++;
        
        // Print status every 60 frames
        if (frame_count % 60 == 0) {
            printf("[FRAME %d] Screen: %d, Running: %d\n", frame_count, g_app_state.current_screen, g_app_state.running);
        }
    }
    
    printf("[LOOP] Main loop ended after %d frames\n", frame_count);
}

static void app_handle_input(void) {
    // For now, just simulate keyboard input
    // In real Switch app, this would use hidScanInput()
    
    // Simple test: exit after some time
    static int input_counter = 0;
    input_counter++;
    
    if (input_counter > 200) {
        g_app_state.running = 0;
    }
}

static void app_render(void) {
    ui_clear_screen();
    ui_draw_screen(g_app_state.current_screen);
    ui_flip_buffer();
}

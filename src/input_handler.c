#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "input_handler.h"

int input_handler_init(void) {
    printf("[Input] Initializing input handler (stub)\n");
    return 0;
}

void input_handler_deinit(void) {
    printf("[Input] Deinitializing input handler\n");
}

int input_handler_read(XCloudInput* input) {
    if (!input) return -1;
    
    memset(input, 0, sizeof(XCloudInput));
    
    // Stub: return empty input for now
    printf("[Input] read() called (stub)\n");
    return 0;
}

int input_handler_encode_for_xbox(XCloudInput* input, unsigned char* data_out, int data_out_size) {
    if (!input || !data_out || data_out_size < 14) return -1;
    
    printf("[Input] encode_for_xbox not yet implemented\n");
    memset(data_out, 0, data_out_size);
    
    return 14;  // Return encoded size
}

void input_handler_print_debug(XCloudInput* input) {
    if (!input) return;
    
    printf("[Input Debug] Buttons: 0x%08X\n", input->buttons);
    printf("[Input Debug] L-Stick: X=%6d Y=%6d\n", input->left_stick_x, input->left_stick_y);
    printf("[Input Debug] R-Stick: X=%6d Y=%6d\n", input->right_stick_x, input->right_stick_y);
    printf("[Input Debug] Triggers: L=%3d R=%3d\n", input->left_trigger, input->right_trigger);
}

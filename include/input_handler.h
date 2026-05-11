#ifndef XCLOUD_INPUT_HANDLER_H
#define XCLOUD_INPUT_HANDLER_H

#include <stdint.h>

typedef struct {
    uint32_t buttons;
    int16_t left_stick_x;
    int16_t left_stick_y;
    int16_t right_stick_x;
    int16_t right_stick_y;
    uint8_t left_trigger;
    uint8_t right_trigger;
} XCloudInput;

// Input handler functions
int input_handler_init(void);
void input_handler_deinit(void);
int input_handler_read(XCloudInput* input);
int input_handler_encode_for_xbox(XCloudInput* input, unsigned char* data_out, int data_out_size);
void input_handler_print_debug(XCloudInput* input);

#endif // XCLOUD_INPUT_HANDLER_H

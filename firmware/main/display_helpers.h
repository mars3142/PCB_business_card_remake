#ifndef PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H
#define PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H
#include <stdint.h>
#include "ws2812_control.h"

extern uint32_t display_buffer[8][15];

void display_write_buffer(struct led_state * new_state);
void display_number(uint32_t number, int start_x, int start_y);
void display_clear(void);

uint32_t hsv_to_rgb(float h, float s, float v);

#endif //PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H

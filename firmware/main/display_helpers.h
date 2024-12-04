#ifndef PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H
#define PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H
#include <stdint.h>
#include "ws2812_control.h"

extern uint32_t display_buffer[8][15];

void display_write_buffer(struct led_state * new_state);

#endif //PCB_BUSINESS_CARD_REMAKE_DISPLAY_HELPERS_H

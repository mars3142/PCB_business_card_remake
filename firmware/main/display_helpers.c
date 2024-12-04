#include "display_helpers.h"

uint32_t display_buffer[8][15] = {
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0xE5FF00, 0xFFFFFF, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0xE5FF00, 0xFFF000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00, 0x0CFF00, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x0CFF00}
};

// Function to limit a color component, R, G, B separately
uint8_t limit_brightness(uint8_t color, uint8_t max_brightness) {
    return (color * max_brightness) / 255;
}

// works on the display_buffer array
// adjusts ordering -- odd row reversed
// adjusts brightness
// writes to the new state buffer passed in as a pointer
void display_write_buffer(struct led_state * new_state){
    int led_num = 0;
    // convert the buffer to our screen xy
    for(int i = 0; i < 8; i++){
        for (int j = 0; j < 15; j++){

            // break out the r g b
            uint8_t r = display_buffer[i][j] >> 16;
            uint8_t g = display_buffer[i][j] >> 8;
            uint8_t b = display_buffer[i][j] >> 0;

            if(i % 2 != 0){
                // odd, have to reverse it
                r = display_buffer[i][14 - j] >> 16;
                g = display_buffer[i][14 - j] >> 8;
                b = display_buffer[i][14 - j] >> 0;
            }


            // Apply brightness limit
            r = limit_brightness(r, 30);
            g = limit_brightness(g, 30);
            b = limit_brightness(b, 30);

            // re-arrange rgb to grb and write it to the array
            new_state->leds[led_num] = (((uint32_t)g) << 16) | (((uint32_t)r) << 8) | (((uint32_t)b) << 0);
            led_num++;
        }
    }
}
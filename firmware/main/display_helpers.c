#include "display_helpers.h"
#include "esp_log.h"

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

// Define the digit patterns (5x3 pixels each)
const uint32_t digit_patterns[10][5] = {
        {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
        {0x2, 0x2, 0x2, 0x2, 0x2}, // 1
        {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
        {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
        {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
        {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
        {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
        {0x7, 0x1, 0x1, 0x1, 0x1}, // 7
        {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
        {0x7, 0x5, 0x7, 0x1, 0x7}  // 9
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

// converts a uint32_t number to separate digits and writes them to the global buffer
void display_number(uint32_t number, int start_x, int start_y) {
    int digits[4] = {0};
    int digit_count = 0;

    // Handle the case when number is 0
    if (number == 0) {
        digits[0] = 0;
        digit_count = 1;
    } else {
        // Extract digits
        while (number > 0 && digit_count < 4) {
            digits[digit_count++] = number % 10;
            number /= 10;
        }
    }

    // Display digits from left to right
    for (int i = digit_count - 1; i >= 0; i--) {
        int digit = digits[i];
        int x = start_x + (digit_count - 1 - i) * 4; // 4 columns per digit including space

        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (digit_patterns[digit][row] & (0x4 >> col)) {
                    display_buffer[start_y + row][x + col] = 0x3F3FFF;
                }
            }
        }
    }
}

// clears the entire buffer
void display_clear(void){
    for (int i = 0; i < 8; i++) {
        for (int b = 0; b < 15; b++) {
            display_buffer[i][b] = 0x000000;
        }
    }
}
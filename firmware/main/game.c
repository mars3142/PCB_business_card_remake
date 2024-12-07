#include "game.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// game speed
uint32_t game_speed = 10;

// global game position -- needs to be set 1 as init
uint32_t game_position = 1;

// pipe positioning
uint32_t game_pipe_position = 0;
uint32_t game_last_pipe_pos = 0xFFFF; // init needs to be forever ago

// game status
uint32_t game_over = 0;
uint32_t game_started = 0;

float game_player_velocity = 0.0f;
float game_player_pos = 4;


uint32_t game_increase_speed_threshold = 200;
uint32_t game_last_button_state = 0;

// colors
#define PIPE_COLOR 0x00FF00
float hue = 0.61f;
#define HUE_SPEED 0.0001f  // Adjust this value to change transition speed

// Function declarations
void game_generate_pipe(uint8_t * pipe);
void game_add_pipe(void);
void game_shift_pipes(void);
void game_compile_buffer(uint32_t buffer[8][15]);
bool game_check_collision(void);
void game_animate_player(void);
void game_animate_background(void);
void game_run(void);


uint32_t game_background[8][15] = {
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000},
        {0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000, 0x200000}
};

uint32_t game_pipes[8][15] = {
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000}
};


// generates a random pipe with an opening
void game_generate_pipe(uint8_t * pipe){
    // gap position can only by at 0 through 5
    uint8_t gap_position = esp_random() % (4 + 1);

    // fill the pipe with 1s
    for(int i = 0; i < 8; i++){
        pipe[i] = 1;
    }

    // open the gap
    for(int i = 0; i < 4; i++){
        pipe[i + gap_position] = 0;
    }
}

// adds the pipe to the pipes layer
void game_add_pipe(void){
    uint8_t pipe[8];

    game_generate_pipe(pipe);

    // add a new pipe
    for(int i = 0; i < 8; i++){
        if(pipe[i] != 0x00)
            game_pipes[i][14] = PIPE_COLOR;
    }

}

// shifts the entire pipes layer to the left
void game_shift_pipes() {
    for (int row = 0; row < 8; row++) {
        // Shift elements to the left
        for (int col = 0; col < 15; col++) {
            game_pipes[row][col] = game_pipes[row][col + 1];
        }
        // Clear the last element
        game_pipes[row][14] = 0x000000;
    }

}

// combines all the layers into a single buffer that we can display
void game_compile_buffer(uint32_t buffer[8][15]){

    // pipes
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            // write the buffer with the background
            buffer[row][col] = game_background[row][col];
            // If the pipe value is non-zero overwrite the background
            if (game_pipes[row][col] != 0x00) {
                buffer[row][col] = game_pipes[row][col];
            }
        }
    }

    // player
    // fixed x position
    buffer[(int)game_player_pos][1] = 0xFFFFFF;
}


#define GRAVITY 0.038f  // magic number -- started with 0.098f m/s
#define JUMP_VELOCITY -0.38f  // Negative because y-axis is inverted
#define MAX_FALL_VELOCITY 0.3f

#define BACKGROUND_STAR_COLOR 0x0F000F
#define BACKGROUND_STAR_CHANCE 50 // 1 in 50 chance of a star appearing


// Add this function for collision detection
bool game_check_collision() {
    int player_x = 1;
    int player_y = (int)game_player_pos;

    // Check if the player's position overlaps with a pipe
    if (game_pipes[player_y][player_x] == PIPE_COLOR) {
        return true;
    }

    return false;
}

// moves the player according to physics and user input
void game_animate_player(void) {
    uint32_t button_pressed = 0;

    // Check for button press (rising edge detection)
    if (gpio_get_level(1) == 1) {
        if (game_last_button_state == 0) {
            game_last_button_state = 1;
            button_pressed = 1;
        }
    } else {
        game_last_button_state = 0;
    }

    // Apply jump velocity if button is pressed
    if (button_pressed) {
        game_player_velocity = JUMP_VELOCITY;
    }

    // Apply gravity
    game_player_velocity += GRAVITY;

    // Limit fall speed
    if (game_player_velocity > MAX_FALL_VELOCITY) {
        game_player_velocity = MAX_FALL_VELOCITY;
    }

    // Update player position
    game_player_pos += game_player_velocity;

    // Bound the player position
    if (game_player_pos > 7) {
        game_player_pos = 7;
        game_player_velocity = 0;  // Stop vertical movement when hitting the ceiling
    } else if (game_player_pos < 0) {
        game_player_pos = 0;
        game_player_velocity = 0;  // Stop vertical movement when hitting the floor
    }
}


void game_reset(void) {
    //reset hue
    hue = 0.63f;

    // Reset game speed
    game_speed = 10;

    // Reset game position
    game_position = 1;

    // Reset pipe positioning
    game_pipe_position = 0;
    game_last_pipe_pos = 0xFFFF;

    // Reset game status
    game_over = 0;
    game_started = 0;

    // Reset player position and velocity
    game_player_velocity = 0.0f;
    game_player_pos = 4;

    // Reset button state
    game_last_button_state = 0;

    // Clear all pipes
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            game_pipes[row][col] = 0x000000;
        }
    }

    // Reset background (optional, depending on if you want to keep the star animation)
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            game_background[row][col] = 0x200000;
        }
    }

    ESP_LOGI("game", "Game reset");
}

// Function to convert HSV to RGB
uint32_t hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h * 6, 2) - 1));
    float m = v - c;
    float r, g, b;

    if (h < 1.0f/6.0f) {
        r = c; g = x; b = 0;
    } else if (h < 2.0f/6.0f) {
        r = x; g = c; b = 0;
    } else if (h < 3.0f/6.0f) {
        r = 0; g = c; b = x;
    } else if (h < 4.0f/6.0f) {
        r = 0; g = x; b = c;
    } else if (h < 5.0f/6.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }

    uint8_t r_int = (uint8_t)((r + m) * 255);
    uint8_t g_int = (uint8_t)((g + m) * 255);
    uint8_t b_int = (uint8_t)((b + m) * 255);

    return (r_int << 16) | (g_int << 8) | b_int;
}

void game_animate_background() {
    uint32_t new_background[8][15];;

    // Calculate the current color based on adjusted hue and saturation
    uint32_t current_color = hsv_to_rgb(hue, 1.0f, 0.08f);


    // Initialize new_background with the current color
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 15; col++) {
            new_background[row][col] = current_color;
        }
    }

    // Shift background stars to the left
    for (int row = 0; row < 8; row++) {
        for (int col = 1; col < 15; col++) {
            if (game_background[row][col] == BACKGROUND_STAR_COLOR) {
                new_background[row][col-1] = BACKGROUND_STAR_COLOR;
            }
        }
    }

    // Randomly add new stars on the right side
    for (int row = 0; row < 8; row++) {
        if (esp_random() % BACKGROUND_STAR_CHANCE == 0) {
            new_background[row][14] = BACKGROUND_STAR_COLOR;
        }
    }

    // Copy new_background to game_background
    memcpy(game_background, new_background, sizeof(game_background));

    // Update hue for next frame and stop at 1.0f (red)
    if(hue < 1.0f)
        hue += HUE_SPEED;

}

uint32_t game_get_game_over(void){
    return game_over;
}

uint32_t game_get_score(void){
    return game_position / 10; //
}

// advances the entire gameplay
void game_run(void){

    if(game_over == 0){
        // increase the game speed
        if(game_position % 300 == 0){
            // if the game speed is above the threshold, decrease to speed it up
            if(game_speed > 3)
                game_speed--;

            ESP_LOGI("game", "frame: %lu, increasing speed to %lu", game_position, game_speed);

        }

        // shift the pipes
        if(game_position % game_speed == 0){

            // shift the pipes
            game_shift_pipes();

            // increment the pipe scroll position
            game_pipe_position++;
        }

        // make the gap random between 5 and 8
        if(game_pipe_position - game_last_pipe_pos > (esp_random() % 4 + 5)){
            // add pipe
            game_add_pipe();

            game_last_pipe_pos = game_pipe_position;
        }

        // animate the player
        game_animate_player();

        // animate the background
        game_animate_background();

        // increment the game position -- doubles as points
        game_position++;

        // Check for collision
        if (game_check_collision()) {
            game_over = 1;

            // display score
            ESP_LOGI("game", "Game Over! Score: %lu", game_position);

        }
    } else {
        // display the score
        // clear all pipes
        // clear player position
    }
}

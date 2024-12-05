#include "game.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "esp_log.h"

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


//#define GRAVITY 0.098f  // Adjusted for 50Hz (9.8 m/s^2 / 100)
#define GRAVITY 0.038f  // Adjusted for 50Hz (9.8 m/s^2 / 100)
#define JUMP_VELOCITY -0.38f  // Negative because y-axis is inverted
#define MAX_FALL_VELOCITY 0.3f


// moves the player according to physics and user input
void game_animate_player(void) {
    uint32_t button_pressed = 0;

    // Check for button press (rising edge detection)
    if (gpio_get_level(1) == 1) {
        if (game_last_button_state == 0) {
            game_last_button_state = 1;
            button_pressed = 1;
            ESP_LOGI("game", "Button pressed");
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


// advances the entire gameplay
void game_run(void){

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

    // increment the game position
    game_position++;

    // animate the background
    // points
    // collision
}

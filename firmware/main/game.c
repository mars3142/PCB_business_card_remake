#include "game.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "esp_log.h"

uint32_t game_speed = 100;
uint32_t game_position = 0;
uint32_t game_last_pipe_pos = 16; // init needs to be forever ago
uint32_t game_over = 0;
uint32_t game_started = 0;
int game_player_pos = 4;

// resposnible for player jump action
#define JUMP_STRENGTH 1
#define GRAVITY 1

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

void game_add_pipe(void){
    uint8_t pipe[8];

    game_generate_pipe(pipe);

    // make the gap random between 5 and 8
    if(game_position - game_last_pipe_pos > (esp_random() % 4 + 5)){
        // add a new pipe
        for(int i = 0; i < 8; i++){
            if(pipe[i] != 0x00)
                game_pipes[i][14] = PIPE_COLOR;
        }

        game_last_pipe_pos = game_position;
    }
}

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
    buffer[game_player_pos][1] = 0xFFFFFF;
}

uint8_t game_speed_enable(void){
    if(game_position % 10){
        return 1;
    } else
        return 0;
}

void game_animate_player(void){

    // if button is pressed, add energy
    if(gpio_get_level(1) == 1){
        game_player_pos -= JUMP_STRENGTH;
    }
    else {
        game_player_pos += GRAVITY;
    }

    // bound the player to min 0 and max 8 position
    if(game_player_pos > 7)
        game_player_pos = 7;
    else if(game_player_pos < 0)
        game_player_pos = 0;

}

void game_run(void){

    // shift the pipes
    game_shift_pipes();

    // add pipe
    game_add_pipe();

    // animate the player
    game_animate_player();

    // increment the game position
    game_position++;

    // animate the background
    // points
    // collision
}

#ifndef PCB_BUSINESS_CARD_REMAKE_GAME_H
#define PCB_BUSINESS_CARD_REMAKE_GAME_H

#include <stdint.h>

extern uint32_t game_background[8][15];
extern uint32_t game_pipes[8][15];

void game_compile_buffer(uint32_t buffer[8][15]);
void game_run(void);


#endif //PCB_BUSINESS_CARD_REMAKE_GAME_H

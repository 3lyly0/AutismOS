#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "types.h"
#include "desktop.h"

typedef struct {
    sint32 value1;
    sint32 value2;
    sint32 result;
    char display[32];
    char operator;
    uint8 has_operator;
    uint8 new_number;
} calc_state_t;

window_t* calculator_create(void);
void calculator_draw(window_t* win);
void calculator_handle_key(window_t* win, char key);

#endif

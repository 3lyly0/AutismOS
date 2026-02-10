#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"
#include "isr.h"

void mouse_init();
void draw_pointer(int x, int y);
void clear_pointer(int x, int y);

// Get mouse state
int mouse_get_x(void);
int mouse_get_y(void);
uint8 mouse_get_buttons(void);

#endif
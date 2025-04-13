#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"
#include "isr.h"

void mouse_init();
void draw_pointer(int x, int y);
void clear_pointer(int x, int y);


#endif
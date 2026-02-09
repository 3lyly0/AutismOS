#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

void clear_screen();
void print(const char *str);
void update_cursor();
void print_hex(uint32_t num);

#endif
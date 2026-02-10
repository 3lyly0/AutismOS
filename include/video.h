#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

void clear_screen();
void print(const char *str);
void update_cursor();
void print_hex(uint32_t num);
void debug_print(const char *str);  // Print to serial only (for boot debug)
void debug_print_hex(uint32_t num); // Print hex to serial only

#endif
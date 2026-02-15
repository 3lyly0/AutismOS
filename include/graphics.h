#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

// Graphics primitives for Step 9 interactive UI

// Color definitions (RGB format for VGA text mode attributes)
#define COLOR_BLACK         0x00
#define COLOR_BLUE          0x01
#define COLOR_GREEN         0x02
#define COLOR_CYAN          0x03
#define COLOR_RED           0x04
#define COLOR_MAGENTA       0x05
#define COLOR_BROWN         0x06
#define COLOR_LIGHT_GRAY    0x07
#define COLOR_DARK_GRAY     0x08
#define COLOR_LIGHT_BLUE    0x09
#define COLOR_LIGHT_GREEN   0x0A
#define COLOR_LIGHT_CYAN    0x0B
#define COLOR_LIGHT_RED     0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW        0x0E
#define COLOR_WHITE         0x0F

// Initialize graphics subsystem
void graphics_init(void);

// Drawing primitives
void draw_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color);
void draw_filled_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color);
void draw_text(uint32 x, uint32 y, const char* text, uint8 color);
void draw_char(uint32 x, uint32 y, char ch, uint8 color);
void draw_cursor(uint32 x, uint32 y, uint8 visible);

// Enhanced drawing functions
void draw_text_with_bg(uint32 x, uint32 y, const char* text, uint8 fg_color, uint8 bg_color);
void draw_char_with_bg(uint32 x, uint32 y, char ch, uint8 fg_color, uint8 bg_color);
void draw_shadow_box(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color);

// Screen operations
void graphics_clear_screen(uint8 color);
void graphics_clear_region(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color);

#endif

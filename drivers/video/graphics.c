#include "graphics.h"
#include "video.h"
#include "types.h"

#define VIDEO_MEMORY ((volatile char *)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

// Initialize graphics subsystem
void graphics_init(void) {
    // No special initialization needed for VGA text mode
}

// Helper to make color attribute byte
static inline uint8 make_color_attr(uint8 fg, uint8 bg) {
    return (bg << 4) | (fg & 0x0F);
}

// Clear entire screen with a color
void graphics_clear_screen(uint8 color) {
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video[i * 2] = ' ';
        video[i * 2 + 1] = attr;
    }
}

// Clear a region of the screen
void graphics_clear_region(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    
    for (uint32 row = y; row < y + h && row < SCREEN_HEIGHT; row++) {
        for (uint32 col = x; col < x + w && col < SCREEN_WIDTH; col++) {
            int index = (row * SCREEN_WIDTH + col) * 2;
            video[index] = ' ';
            video[index + 1] = attr;
        }
    }
}

// Draw a filled rectangle
void draw_filled_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, color);  // Same fg and bg for solid fill
    
    for (uint32 row = y; row < y + h && row < SCREEN_HEIGHT; row++) {
        for (uint32 col = x; col < x + w && col < SCREEN_WIDTH; col++) {
            int index = (row * SCREEN_WIDTH + col) * 2;
            video[index] = ' ';
            video[index + 1] = attr;
        }
    }
}

// Draw a rectangle outline
void draw_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    
    // Top and bottom borders
    for (uint32 col = x; col < x + w && col < SCREEN_WIDTH; col++) {
        // Top
        if (y < SCREEN_HEIGHT) {
            int index = (y * SCREEN_WIDTH + col) * 2;
            video[index] = '-';
            video[index + 1] = attr;
        }
        // Bottom
        if (y + h - 1 < SCREEN_HEIGHT) {
            int index = ((y + h - 1) * SCREEN_WIDTH + col) * 2;
            video[index] = '-';
            video[index + 1] = attr;
        }
    }
    
    // Left and right borders
    for (uint32 row = y; row < y + h && row < SCREEN_HEIGHT; row++) {
        // Left
        if (x < SCREEN_WIDTH) {
            int index = (row * SCREEN_WIDTH + x) * 2;
            video[index] = '|';
            video[index + 1] = attr;
        }
        // Right
        if (x + w - 1 < SCREEN_WIDTH) {
            int index = (row * SCREEN_WIDTH + x + w - 1) * 2;
            video[index] = '|';
            video[index + 1] = attr;
        }
    }
    
    // Corners
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        int index = (y * SCREEN_WIDTH + x) * 2;
        video[index] = '+';
        video[index + 1] = attr;
    }
    if (x + w - 1 < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        int index = (y * SCREEN_WIDTH + x + w - 1) * 2;
        video[index] = '+';
        video[index + 1] = attr;
    }
    if (x < SCREEN_WIDTH && y + h - 1 < SCREEN_HEIGHT) {
        int index = ((y + h - 1) * SCREEN_WIDTH + x) * 2;
        video[index] = '+';
        video[index + 1] = attr;
    }
    if (x + w - 1 < SCREEN_WIDTH && y + h - 1 < SCREEN_HEIGHT) {
        int index = ((y + h - 1) * SCREEN_WIDTH + x + w - 1) * 2;
        video[index] = '+';
        video[index + 1] = attr;
    }
}

// Draw a single character
void draw_char(uint32 x, uint32 y, char ch, uint8 color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    int index = (y * SCREEN_WIDTH + x) * 2;
    
    video[index] = ch;
    video[index + 1] = attr;
}

// Draw text string
void draw_text(uint32 x, uint32 y, const char* text, uint8 color) {
    if (!text || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    uint32 col = x;
    
    while (*text && col < SCREEN_WIDTH) {
        int index = (y * SCREEN_WIDTH + col) * 2;
        video[index] = *text;
        video[index + 1] = attr;
        text++;
        col++;
    }
}

// Draw cursor at position
void draw_cursor(uint32 x, uint32 y, uint8 visible) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    int index = (y * SCREEN_WIDTH + x) * 2;
    
    if (visible) {
        // Draw cursor as underscore with inverse colors
        video[index] = '_';
        video[index + 1] = make_color_attr(COLOR_BLACK, COLOR_WHITE);
    } else {
        // Clear cursor
        video[index] = ' ';
        video[index + 1] = make_color_attr(COLOR_WHITE, COLOR_BLACK);
    }
}

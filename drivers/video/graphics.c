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

// Clear entire screen with a color - optimized
void graphics_clear_screen(uint8 color) {
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    
    // Optimize by writing both character and attribute at once
    volatile uint16 *video16 = (volatile uint16 *)VIDEO_MEMORY;
    uint16 fill_value = (' ' | (attr << 8));
    
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        video16[i] = fill_value;
    }
}

// Clear a region of the screen - optimized
void graphics_clear_region(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    uint16 fill_value = (' ' | (attr << 8));
    
    for (uint32 row = y; row < y + h && row < SCREEN_HEIGHT; row++) {
        volatile uint16 *video16 = (volatile uint16 *)(VIDEO_MEMORY + (row * SCREEN_WIDTH + x) * 2);
        uint32 count = (x + w <= SCREEN_WIDTH) ? w : (SCREEN_WIDTH - x);
        
        for (uint32 i = 0; i < count; i++) {
            video16[i] = fill_value;
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

// Draw a rectangle outline with enhanced box-drawing characters
void draw_rect(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(color, COLOR_BLACK);
    
    // Validate minimum dimensions
    if (w < 2 || h < 2) {
        return;
    }
    
    // Use box-drawing characters for better visuals
    // ─ (0xC4) horizontal, │ (0xB3) vertical
    // ┌ (0xDA) top-left, ┐ (0xBF) top-right
    // └ (0xC0) bottom-left, ┘ (0xD9) bottom-right
    
    // Top and bottom borders
    for (uint32 col = x + 1; col < x + w - 1 && col < SCREEN_WIDTH; col++) {
        // Top
        if (y < SCREEN_HEIGHT) {
            int index = (y * SCREEN_WIDTH + col) * 2;
            video[index] = 0xC4;  // ─
            video[index + 1] = attr;
        }
        // Bottom
        if (y + h - 1 < SCREEN_HEIGHT) {
            int index = ((y + h - 1) * SCREEN_WIDTH + col) * 2;
            video[index] = 0xC4;  // ─
            video[index + 1] = attr;
        }
    }
    
    // Left and right borders
    for (uint32 row = y + 1; row < y + h - 1 && row < SCREEN_HEIGHT; row++) {
        // Left
        if (x < SCREEN_WIDTH) {
            int index = (row * SCREEN_WIDTH + x) * 2;
            video[index] = 0xB3;  // │
            video[index + 1] = attr;
        }
        // Right
        if (x + w - 1 < SCREEN_WIDTH) {
            int index = (row * SCREEN_WIDTH + x + w - 1) * 2;
            video[index] = 0xB3;  // │
            video[index + 1] = attr;
        }
    }
    
    // Corners with box-drawing characters
    if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        int index = (y * SCREEN_WIDTH + x) * 2;
        video[index] = 0xDA;  // ┌
        video[index + 1] = attr;
    }
    if (x + w - 1 < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
        int index = (y * SCREEN_WIDTH + x + w - 1) * 2;
        video[index] = 0xBF;  // ┐
        video[index + 1] = attr;
    }
    if (x < SCREEN_WIDTH && y + h - 1 < SCREEN_HEIGHT) {
        int index = ((y + h - 1) * SCREEN_WIDTH + x) * 2;
        video[index] = 0xC0;  // └
        video[index + 1] = attr;
    }
    if (x + w - 1 < SCREEN_WIDTH && y + h - 1 < SCREEN_HEIGHT) {
        int index = ((y + h - 1) * SCREEN_WIDTH + x + w - 1) * 2;
        video[index] = 0xD9;  // ┘
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

// Draw cursor at position with enhanced visibility
void draw_cursor(uint32 x, uint32 y, uint8 visible) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    int index = (y * SCREEN_WIDTH + x) * 2;
    
    if (visible) {
        // Enhanced cursor: use block character (█ 0xDB) for better visibility
        video[index] = 0xDB;  // █ full block
        video[index + 1] = make_color_attr(COLOR_WHITE, COLOR_BLACK);
    } else {
        // Clear cursor
        video[index] = ' ';
        video[index + 1] = make_color_attr(COLOR_WHITE, COLOR_BLACK);
    }
}

// Draw text with custom foreground and background colors
void draw_text_with_bg(uint32 x, uint32 y, const char* text, uint8 fg_color, uint8 bg_color) {
    if (!text || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(fg_color, bg_color);
    uint32 col = x;
    
    while (*text && col < SCREEN_WIDTH) {
        int index = (y * SCREEN_WIDTH + col) * 2;
        video[index] = *text;
        video[index + 1] = attr;
        text++;
        col++;
    }
}

// Draw a single character with custom foreground and background colors
void draw_char_with_bg(uint32 x, uint32 y, char ch, uint8 fg_color, uint8 bg_color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
        return;
    }
    
    volatile char *video = VIDEO_MEMORY;
    uint8 attr = make_color_attr(fg_color, bg_color);
    int index = (y * SCREEN_WIDTH + x) * 2;
    
    video[index] = ch;
    video[index + 1] = attr;
}

// Draw a box with shadow effect for depth perception
void draw_shadow_box(uint32 x, uint32 y, uint32 w, uint32 h, uint8 color) {
    // Draw main box
    draw_rect(x, y, w, h, color);
    
    // Draw shadow on right and bottom edges (if within bounds)
    volatile char *video = VIDEO_MEMORY;
    uint8 shadow_attr = make_color_attr(COLOR_DARK_GRAY, COLOR_BLACK);
    
    // Right shadow
    for (uint32 row = y + 1; row <= y + h && row < SCREEN_HEIGHT; row++) {
        if (x + w < SCREEN_WIDTH) {
            int index = (row * SCREEN_WIDTH + x + w) * 2;
            video[index] = 0xB0;  // ░ light shade
            video[index + 1] = shadow_attr;
        }
    }
    
    // Bottom shadow
    for (uint32 col = x + 1; col <= x + w && col < SCREEN_WIDTH; col++) {
        if (y + h < SCREEN_HEIGHT) {
            int index = ((y + h) * SCREEN_WIDTH + col) * 2;
            video[index] = 0xB0;  // ░ light shade
            video[index + 1] = shadow_attr;
        }
    }
}

// Draw a progress bar with percentage
void draw_progress_bar(uint32 x, uint32 y, uint32 w, uint32 percent, uint8 color) {
    if (y >= SCREEN_HEIGHT || w < 2) {
        return;
    }
    
    // Clamp percentage to 0-100
    if (percent > 100) {
        percent = 100;
    }
    
    volatile char *video = VIDEO_MEMORY;
    uint8 bar_attr = make_color_attr(color, COLOR_BLACK);
    uint8 empty_attr = make_color_attr(COLOR_DARK_GRAY, COLOR_BLACK);
    
    // Calculate filled width
    uint32 filled = (w * percent) / 100;
    
    // Draw filled portion with solid blocks
    for (uint32 col = x; col < x + filled && col < SCREEN_WIDTH; col++) {
        int index = (y * SCREEN_WIDTH + col) * 2;
        video[index] = 0xDB;  // █ full block
        video[index + 1] = bar_attr;
    }
    
    // Draw empty portion with light blocks
    for (uint32 col = x + filled; col < x + w && col < SCREEN_WIDTH; col++) {
        int index = (y * SCREEN_WIDTH + col) * 2;
        video[index] = 0xB0;  // ░ light shade
        video[index + 1] = empty_attr;
    }
}

#include <stdint.h>
#include "video.h"
#include "io_ports.h"

#define VIDEO_MEMORY ((volatile char *)0xB8000)
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

static int cursor_x = 0;
static int cursor_y = 0;

void scroll_screen() {
    volatile char *video_memory = VIDEO_MEMORY;

    for (int y = 1; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int from_index = (y * SCREEN_WIDTH + x) * 2;
            int to_index = ((y - 1) * SCREEN_WIDTH + x) * 2;
            video_memory[to_index] = video_memory[from_index];
            video_memory[to_index + 1] = video_memory[from_index + 1];
        }
    }

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int index = ((SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x) * 2;
        video_memory[index] = ' ';
        video_memory[index + 1] = 0x07;
    }
}

void clear_screen() {
    volatile char *video = (volatile char *)VIDEO_MEMORY;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i++) {
        video[i] = 0;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void print(const char *str) {
    volatile char *video_memory = (volatile char *)0xB8000;
    static unsigned int row = 0, col = 0;

    while (*str) {
        // Also send to serial port for debugging
        outportb(0x3F8, (unsigned char)*str);
        
        if (*str == '\n') {
            row++;
            col = 0;
        } else if (*str == '\b') {
            if (col > 0) {
                col--;
            } else if (row > 0) {
                row--;
                col = 79;
            }
            video_memory[(row * SCREEN_WIDTH + col) * 2] = ' ';
            video_memory[(row * SCREEN_WIDTH + col) * 2 + 1] = 0x0F;
        } else {
            video_memory[(row * SCREEN_WIDTH + col) * 2] = *str;
            video_memory[(row * SCREEN_WIDTH + col) * 2 + 1] = 0x0F;
            col++;
            if (col >= SCREEN_WIDTH) {
                col = 0;
                row++;
            }
        }
        if (row >= SCREEN_HEIGHT) {
            scroll_screen();
            row = SCREEN_HEIGHT - 1;
        }
        str++;
    }

    update_cursor(row, col);
}

void update_cursor(int row, int col) {
    unsigned short position = (row * SCREEN_WIDTH) + col;

    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (unsigned char)(position & 0xFF));
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}


void print_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';

    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;
    }

    print("0x");
    print(buffer);
}

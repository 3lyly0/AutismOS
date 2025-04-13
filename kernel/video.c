#include <stdint.h>
#include "video.h"
#include "io_ports.h"

#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

static int cursor_x = 0;
static int cursor_y = 0;

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
            clear_screen();
            row = 0;
            col = 0;
        }
        update_cursor(row, col);
        str++;
    }
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

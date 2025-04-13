#include <stdint.h>
#include "kernel.h"
#include "string.h"
#include "gdt.h"
#include "idt.h"
#include "mouse.h"
#include "keyboard.h"
#include "io_ports.h"


void update_cursor(int row, int col) {
    unsigned short position = (row * 80) + col;

    outportb(0x3D4, 0x0F);
    outportb(0x3D5, (unsigned char)(position & 0xFF));
    outportb(0x3D4, 0x0E);
    outportb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
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
            video_memory[(row * 80 + col) * 2] = ' ';
            video_memory[(row * 80 + col) * 2 + 1] = 0x0F;
        } else {
            video_memory[(row * 80 + col) * 2] = *str;
            video_memory[(row * 80 + col) * 2 + 1] = 0x0F;
            col++;
            if (col >= 80) {
                col = 0;
                row++;
            }
        }
        if (row >= 25) {
            clear_screen();
            row = 0;
            col = 0;
        }
        update_cursor(row, col);
        str++;
    }
}

void clear_screen() {
    uint16_t *video_memory = (uint16_t *)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = (0x0F << 8) | ' ';
    }
    update_cursor(0, 0);
}

void kmain() {
    gdt_init();
    idt_init();
    keyboard_init();
    mouse_init();
    clear_screen();
    print("Welcome to AutismOS!\n");
    print("Press any key to see it on the screen...\n");

    while (1) {
        char c = kb_getchar();
        if (c != 0) {
            char str[2] = {c, '\0'};
            print(str);
        }
    }
}
#include <stdint.h>
#include <stdio.h>
#include "kernel.h"
#include "mouse.h"
#include "video.h"
#include "io_ports.h"
#include "isr.h"
#include "string.h"


void mouse_handler(REGISTERS *r __attribute__((unused))) {
    static unsigned char mouse_cycle = 0;
    static char mouse_packet[3];
    static int pointer_x = 40, pointer_y = 12;


    mouse_packet[mouse_cycle++] = inportb(0x60);

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        int x_sign = (mouse_packet[0] & 0x10) ? -1 : 1;
        int y_sign = (mouse_packet[0] & 0x20) ? -1 : 1;

        int x_raw = mouse_packet[1] & 0xFF;
        int y_raw = mouse_packet[2] & 0xFF;


        int x_movement = (x_sign * x_raw / 2) / 2;
        int y_movement = (y_sign * y_raw / 2) / 2;


        clear_pointer(pointer_x, pointer_y);

        pointer_x += x_movement;
        pointer_y -= y_movement;

        if (pointer_x < 0) pointer_x = 0;
        if (pointer_x >= SCREEN_WIDTH) pointer_x = SCREEN_WIDTH - 1;
        if (pointer_y < 0) pointer_y = 0;
        if (pointer_y >= SCREEN_HEIGHT) pointer_y = SCREEN_HEIGHT - 1;

        draw_pointer(pointer_x, pointer_y);
    }
}


static void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inportb(0x64) & 1) == 1) {
                return;
            }
        }
    } else {
        while (timeout--) {
            if ((inportb(0x64) & 2) == 0) {
                return;
            }
        }
    }
}

static void mouse_write(unsigned char data) {
    mouse_wait(1);
    outportb(0x64, 0xD4);
    mouse_wait(1);
    outportb(0x60, data);
}

static unsigned char mouse_read() {
    mouse_wait(0);
    return inportb(0x60);
}


void mouse_init() {
    mouse_wait(1);
    outportb(0x64, 0xA8);

    mouse_wait(1);
    outportb(0x64, 0x20);
    unsigned char status = mouse_read() | 2;
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    isr_register_interrupt_handler(IRQ_BASE + 12, mouse_handler);
}


void clear_pointer(int x, int y) {
    volatile char *video_memory = (volatile char *)0xB8000;
    int index = (y * SCREEN_WIDTH + x) * 2;
    video_memory[index] = ' ';
    video_memory[index + 1] = 0x0F;
}


void draw_pointer(int x, int y) {
    volatile char *video_memory = (volatile char *)0xB8000;
    int index = (y * SCREEN_WIDTH + x) * 2;
    video_memory[index] = 'X';
    video_memory[index + 1] = 0x0F;
}
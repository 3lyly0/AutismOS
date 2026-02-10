#include <stdint.h>
#include "kernel.h"
#include "mouse.h"
#include "video.h"
#include "io_ports.h"
#include "isr.h"
#include "string.h"
#include "desktop.h"

// Global mouse state
static int g_mouse_x = 40;
static int g_mouse_y = 12;
static uint8 g_mouse_buttons = 0;

// Accumulator for fractional movement
static int g_accum_x = 0;
static int g_accum_y = 0;

int mouse_get_x(void) { return g_mouse_x; }
int mouse_get_y(void) { return g_mouse_y; }
uint8 mouse_get_buttons(void) { return g_mouse_buttons; }


void mouse_handler(REGISTERS *r __attribute__((unused))) {
    static unsigned char mouse_cycle = 0;
    static char mouse_packet[3];


    mouse_packet[mouse_cycle++] = inportb(0x60);

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        // Get raw movement values
        int x_move = mouse_packet[1];
        int y_move = mouse_packet[2];

        // Sign extend using bit 4 (X) and bit 5 (Y) of byte 0
        if (mouse_packet[0] & 0x10) x_move -= 256;  // Negative X
        if (mouse_packet[0] & 0x20) y_move -= 256;  // Negative Y

        // Accumulate movement to avoid losing small values
        g_accum_x += x_move;
        g_accum_y += y_move;

        // Apply movement with sensitivity reduction (divide by 3)
        int x_movement = g_accum_x / 3;
        int y_movement = g_accum_y / 3;

        // Keep remainder for next time
        g_accum_x -= x_movement * 3;
        g_accum_y -= y_movement * 3;

        g_mouse_x += x_movement;
        g_mouse_y -= y_movement;

        if (g_mouse_x < 0) g_mouse_x = 0;
        if (g_mouse_x >= SCREEN_WIDTH) g_mouse_x = SCREEN_WIDTH - 1;
        if (g_mouse_y < 0) g_mouse_y = 0;
        if (g_mouse_y >= SCREEN_HEIGHT) g_mouse_y = SCREEN_HEIGHT - 1;

        // Update desktop with mouse movement
        g_mouse_buttons = mouse_packet[0] & 0x07;
        
        // If desktop is initialized, update it
        desktop_t* ds = desktop_get_state();
        if (ds && ds->initialized) {
            ds->mouse_x = g_mouse_x;
            ds->mouse_y = g_mouse_y;
            ds->mouse_buttons = g_mouse_buttons;
            // Draw mouse immediately from IRQ
            extern void desktop_draw_mouse_direct(void);
            desktop_draw_mouse_direct();
        } else {
            // Fallback: draw pointer directly
            static int last_x = 40, last_y = 12;
            clear_pointer(last_x, last_y);
            draw_pointer(g_mouse_x, g_mouse_y);
            last_x = g_mouse_x;
            last_y = g_mouse_y;
        }
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
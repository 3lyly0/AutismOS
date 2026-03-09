#include <stdint.h>
#include "mouse.h"
#include "video.h"
#include "io_ports.h"
#include "isr.h"
#include "desktop.h"

static int g_mouse_x = SCREEN_WIDTH / 2;
static int g_mouse_y = SCREEN_HEIGHT / 2;
static uint8 g_mouse_buttons = 0;
static int g_frac_x = 0;
static int g_frac_y = 0;

int mouse_get_x(void) { return g_mouse_x; }
int mouse_get_y(void) { return g_mouse_y; }
uint8 mouse_get_buttons(void) { return g_mouse_buttons; }

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

void mouse_handler(REGISTERS *r __attribute__((unused))) {
    static uint8 mouse_cycle = 0;
    static int8 mouse_packet[3];

    int8 data = (int8)inportb(0x60);
    mouse_packet[mouse_cycle++] = data;

    if (mouse_cycle < 3) {
        return;
    }

    mouse_cycle = 0;

    if (!(mouse_packet[0] & 0x08)) {
        return;
    }

    if (mouse_packet[0] & 0x40 || mouse_packet[0] & 0x80) {
        return;
    }

    g_frac_x += mouse_packet[1] * 2;
    g_frac_y += mouse_packet[2] * 2;

    int dx = g_frac_x / 3;
    int dy = g_frac_y / 3;

    g_frac_x -= dx * 3;
    g_frac_y -= dy * 3;

    if (dx > 8) dx = 8;
    if (dx < -8) dx = -8;
    if (dy > 8) dy = 8;
    if (dy < -8) dy = -8;

    g_mouse_buttons = mouse_packet[0] & 0x07;
    desktop_update_mouse(dx, dy, g_mouse_buttons);

    desktop_t* ds = desktop_get_state();
    if (ds) {
        g_mouse_x = ds->mouse_x;
        g_mouse_y = ds->mouse_y;
    }
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
    (void)x;
    (void)y;
}

void draw_pointer(int x, int y) {
    (void)x;
    (void)y;
}

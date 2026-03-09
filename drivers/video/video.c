#include <stdint.h>
#include "video.h"
#include "graphics.h"
#include "io_ports.h"

static uint32 g_text_row = 0;
static uint32 g_text_col = 0;

static void serial_write_char(char ch) {
    outportb(0x3F8, (unsigned char)ch);
}

void clear_screen() {
    g_text_row = 0;
    g_text_col = 0;

    if (graphics_is_initialized()) {
        graphics_clear_screen(COLOR_BLACK);
        graphics_present();
    }
}

void update_cursor(int row, int col) {
    (void)row;
    (void)col;
}

void print(const char *str) {
    if (!str) {
        return;
    }

    while (*str) {
        char ch = *str++;
        serial_write_char(ch);

        if (!graphics_is_initialized()) {
            continue;
        }

        if (ch == '\n') {
            g_text_row += 10;
            g_text_col = 0;
            continue;
        }

        if (ch == '\b') {
            if (g_text_col >= 6) {
                g_text_col -= 6;
            }
            continue;
        }

        draw_char(g_text_col, g_text_row, ch, COLOR_WHITE);
        g_text_col += 6;

        if (g_text_col > SCREEN_WIDTH - 6) {
            g_text_col = 0;
            g_text_row += 10;
        }
    }

    if (graphics_is_initialized()) {
        graphics_present();
    }
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

void debug_print(const char *str) {
    if (!str) {
        return;
    }

    while (*str) {
        serial_write_char(*str++);
    }
}

void debug_print_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';

    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;
    }

    debug_print("0x");
    debug_print(buffer);
}

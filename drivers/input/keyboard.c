#include <stdint.h>
#include "keyboard.h"
#include "idt.h"
#include "io_ports.h"
#include "isr.h"
#include "types.h"
#include "string.h"
#include "ux.h"
#include "desktop.h"

// External function to check if desktop mode is active
extern uint8 is_desktop_mode(void);

static BOOL g_caps_lock = FALSE;
static BOOL g_shift_pressed = FALSE;
static BOOL g_alt_pressed = FALSE;
static BOOL g_ctrl_pressed = FALSE;
static BOOL g_extended = FALSE;  // E0 prefix received
char g_ch = 0;

#define SCAN_CODE_KEY_CTRL 0x1D

// Special key codes (not printable)
#define KEY_WIN     0x80  // Windows key (Start menu)
#define KEY_F10     0x81  // F10 (Quit)
#define KEY_F12     0x82  // F12

char g_scan_code_chars[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static int get_scancode() {
    int i, scancode = 0;

    for (i = 1000; i > 0; i++) {
        if ((inportb(KEYBOARD_STATUS_PORT) & 1) == 0) continue;
        scancode = inportb(KEYBOARD_DATA_PORT);
        break;
    }
    if (i > 0)
        return scancode;
    return 0;
}

char alternate_chars(char ch) {
    switch(ch) {
        case '`': return '~';
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case '\\': return '|';
        case ';': return ':';
        case '\'': return '\"';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        default: return ch;
    }
}

void keyboard_handler(REGISTERS *r __attribute__((unused))) {
    int scancode;

    g_ch = 0;
    scancode = get_scancode();
    
    // Handle extended scancode prefix (E0)
    if (scancode == 0xE0) {
        g_extended = TRUE;
        return;  // Wait for next scancode
    }
    
    // Handle extended keys
    if (g_extended) {
        g_extended = FALSE;
        if (!(scancode & 0x80)) {  // Key press, not release
            if (scancode == 0x5B || scancode == 0x5C) {
                // Left or Right Windows key pressed
                g_ch = KEY_WIN;
                if (is_desktop_mode()) {
                    desktop_handle_key(g_ch);
                }
                return;
            }
        }
        return;  // Ignore other extended keys for now
    }
    
    if (scancode & 0x80) {
        // Key released
        if (scancode == (SCAN_CODE_KEY_LEFT_SHIFT | 0x80)) {
            g_shift_pressed = FALSE;
        } else if (scancode == (SCAN_CODE_KEY_ALT | 0x80)) {
            g_alt_pressed = FALSE;
        } else if (scancode == (SCAN_CODE_KEY_CTRL | 0x80)) {
            g_ctrl_pressed = FALSE;
        }
    } else {
        // Key pressed
        switch(scancode) {
            case SCAN_CODE_KEY_CAPS_LOCK:
                g_caps_lock = !g_caps_lock;
                break;

            case SCAN_CODE_KEY_LEFT_SHIFT:
                g_shift_pressed = TRUE;
                break;

            case SCAN_CODE_KEY_ALT:
                g_alt_pressed = TRUE;
                break;

            case SCAN_CODE_KEY_CTRL:
                g_ctrl_pressed = TRUE;
                break;

            default:
                g_ch = g_scan_code_chars[scancode];
                if (g_caps_lock) {
                    if (isalpha(g_ch)) {
                        g_ch = upper(g_ch);
                    }
                }
                if (g_shift_pressed) {
                    g_ch = alternate_chars(g_ch);
                }
                
                // Handle Ctrl+Key combinations
                if (g_ctrl_pressed && g_ch != 0) {
                    // Ctrl+Q = send quit signal (ASCII 17)
                    if (g_ch == 'q' || g_ch == 'Q') {
                        if (is_desktop_mode()) {
                            desktop_handle_key(17);  // Ctrl+Q
                        }
                    }
                    return;
                }
                
                // Handle Alt+Key combinations (Step 8: UX Hotkeys)
                if (g_alt_pressed && g_ch != 0) {
                    ux_handle_hotkey(g_ch);
                }
                // Send to desktop if active
                else if (is_desktop_mode() && g_ch != 0) {
                    desktop_handle_key(g_ch);
                }
                // Step 9: Route input to UX for UI handling
                else if (g_ch != 0) {
                    // Handle Enter key specially
                    if (g_ch == '\n') {
                        ux_handle_enter_key();
                    } else {
                        // Regular character input
                        ux_handle_key_input(g_ch);
                    }
                }
                break;
        }
    }
}

void keyboard_init() {
    isr_register_interrupt_handler(IRQ_BASE + 1, keyboard_handler);
}


char kb_getchar() {
    char c;

    while(g_ch <= 0);
    c = g_ch;
    g_ch = 0;
    return c;
}



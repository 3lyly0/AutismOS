#include <stdint.h>
#include "keyboard.h"
#include "idt.h"
#include "io_ports.h"
#include "isr.h"
#include "types.h"
#include "string.h"
#include "ux.h"
#include "desktop.h"

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
#define KEY_F1      0x83
#define KEY_F2      0x84
#define KEY_F3      0x85
#define KEY_F4      0x86
#define KEY_F5      0x87
#define KEY_TAB     0x88

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

            case 0x3B:
                if (is_desktop_mode()) desktop_handle_key(KEY_F1);
                break;

            case 0x3C:
                if (is_desktop_mode()) desktop_handle_key(KEY_F2);
                break;

            case 0x3D:
                if (is_desktop_mode()) desktop_handle_key(KEY_F3);
                break;

            case 0x3E:
                if (is_desktop_mode()) desktop_handle_key(KEY_F4);
                break;

            case 0x3F:
                if (is_desktop_mode()) desktop_handle_key(KEY_F5);
                break;

            case 0x44:
                if (is_desktop_mode()) desktop_handle_key(KEY_F10);
                break;

            case 0x58:
                if (is_desktop_mode()) desktop_handle_key(KEY_F12);
                break;

            case 0x0F:
                if (is_desktop_mode()) {
                    if (g_alt_pressed) {
                        desktop_handle_key(KEY_TAB);
                    } else {
                        desktop_handle_key('\t');
                    }
                }
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
                    if (is_desktop_mode()) {
                        if (g_ch == 'q' || g_ch == 'Q') desktop_handle_key(17);      // Ctrl+Q
                        else if (g_ch == 'w' || g_ch == 'W') desktop_handle_key(23); // Ctrl+W
                        else if (g_ch == 'n' || g_ch == 'N') desktop_handle_key(14); // Ctrl+N
                        else if (g_ch == 'm' || g_ch == 'M') desktop_handle_key(13); // Ctrl+M
                        else if (g_ch == 't' || g_ch == 'T') desktop_handle_key(20); // Ctrl+T
                        else if (g_ch == 'r' || g_ch == 'R') desktop_handle_key(18); // Ctrl+R
                        else if (g_ch == 'i' || g_ch == 'I') desktop_handle_key(9);  // Ctrl+I
                    }
                    return;
                }
                
                // Global hotkeys remain available regardless of the active shell.
                if (g_alt_pressed && g_ch != 0) {
                    if (is_desktop_mode()) {
                        if (g_ch == '1') desktop_handle_key(KEY_F1);
                        else if (g_ch == '2') desktop_handle_key(KEY_F2);
                        else if (g_ch == '3') desktop_handle_key(KEY_F3);
                        else if (g_ch == '4') desktop_handle_key(KEY_F4);
                        else if (g_ch == 'm' || g_ch == 'M') desktop_handle_key(13);
                        else if (g_ch == 'x' || g_ch == 'X') desktop_handle_key(KEY_F10);
                        else ux_handle_hotkey(g_ch);
                    } else {
                        ux_handle_hotkey(g_ch);
                    }
                }
                else if (is_desktop_mode() && g_ch != 0) {
                    desktop_handle_key(g_ch);
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



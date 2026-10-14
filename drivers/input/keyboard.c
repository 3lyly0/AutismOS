#include <stdint.h>
#include "keyboard.h"
#include "idt.h"
#include "io_ports.h"
#include "isr.h"
#include "types.h"
#include "string.h"
#include "video.h"
#include "kheap.h"
#include "ux.h"
#include "desktop.h"

static BOOL g_caps_lock = FALSE;
static BOOL g_shift_pressed = FALSE;
static BOOL g_alt_pressed = FALSE;
static BOOL g_ctrl_pressed = FALSE;
static BOOL g_extended = FALSE;  // E0 prefix received
char g_ch = 0;

#define KB_BUFFER_SIZE 256
static char g_kb_buffer[KB_BUFFER_SIZE];
static volatile uint16_t g_kb_head = 0;
static volatile uint16_t g_kb_tail = 0;

static void kb_enqueue(char ch) {
    uint16_t next = (g_kb_head + 1) % KB_BUFFER_SIZE;
    if (next != g_kb_tail) {
        g_kb_buffer[g_kb_head] = ch;
        g_kb_head = next;
    }
}

int kb_has_char(void) {
    return g_kb_head != g_kb_tail;
}

#define SCAN_CODE_KEY_CTRL 0x1D

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

    for (i = 1000; i > 0; i--) {
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
            char special = 0;
            if (scancode == 0x5B || scancode == 0x5C) {
                special = KB_KEY_WIN;
            } else if (scancode == 0x48) {
                special = KB_KEY_UP;
            } else if (scancode == 0x50) {
                special = KB_KEY_DOWN;
            } else if (scancode == 0x4B) {
                special = KB_KEY_LEFT;
            } else if (scancode == 0x4D) {
                special = KB_KEY_RIGHT;
            }
            if (special) {
                g_ch = special;
                kb_enqueue(special);
                if (is_desktop_mode()) {
                    desktop_handle_key(special);
                }
            }
        }
        return;
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
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F1);
                break;

            case 0x3C:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F2);
                break;

            case 0x3D:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F3);
                break;

            case 0x3E:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F4);
                break;

            case 0x3F:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F5);
                break;

            case 0x44:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F10);
                break;

            case 0x58:
                if (is_desktop_mode()) desktop_handle_key(KB_KEY_F12);
                break;

            case 0x0F:
                if (is_desktop_mode()) {
                    if (g_alt_pressed) {
                        desktop_handle_key(KB_KEY_TAB);
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
                    if (g_ch == 'c' || g_ch == 'C') {
                        g_ch = KB_KEY_CTRL_C;
                        kb_enqueue(KB_KEY_CTRL_C);
                        return;
                    }
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
                    ux_handle_hotkey(g_ch);
                }
                else if (is_desktop_mode() && g_ch != 0) {
                    desktop_handle_key(g_ch);
                }

                if (g_ch != 0) {
                    kb_enqueue(g_ch);
                }
                break;
        }
    }
}

void keyboard_init(void) {
    isr_register_interrupt_handler(IRQ_BASE + 1, keyboard_handler);
}

char kb_getchar(void) {
    while (g_kb_head == g_kb_tail) {
        asm volatile("hlt");
    }
    char c = g_kb_buffer[g_kb_tail];
    g_kb_tail = (g_kb_tail + 1) % KB_BUFFER_SIZE;
    g_ch = c;
    return c;
}

int readline_buf(char* buffer, int max_len, const char* prompt) {
    if (!buffer || max_len <= 0) return -1;
    if (prompt) {
        print(prompt);
    }

    int len = 0;
    int pos = 0;
    buffer[0] = '\0';

    while (1) {
        char ch = kb_getchar();

        // Ctrl+C: Cancel current input
        if (ch == KB_KEY_CTRL_C) {
            print("^C\n");
            buffer[0] = '\0';
            return -1;
        }

        // Enter: submit line
        if (ch == '\n' || ch == '\r') {
            print("\n");
            buffer[len] = '\0';
            return len;
        }

        // Backspace
        if (ch == '\b') {
            if (pos > 0) {
                if (pos == len) {
                    pos--;
                    len--;
                    buffer[len] = '\0';
                    print("\b \b");
                } else {
                    for (int i = pos - 1; i < len - 1; i++) {
                        buffer[i] = buffer[i + 1];
                    }
                    pos--;
                    len--;
                    buffer[len] = '\0';
                    print("\b");
                    for (int i = pos; i < len; i++) {
                        char s[2] = {buffer[i], '\0'};
                        print(s);
                    }
                    print(" ");
                    for (int i = len + 1; i > pos; i--) {
                        print("\b");
                    }
                }
            }
            continue;
        }

        // Left arrow
        if (ch == KB_KEY_LEFT) {
            if (pos > 0) {
                pos--;
                print("\b");
            }
            continue;
        }

        // Right arrow
        if (ch == KB_KEY_RIGHT) {
            if (pos < len) {
                char s[2] = {buffer[pos], '\0'};
                print(s);
                pos++;
            }
            continue;
        }

        // Regular printable character
        if (ch >= 32 && ch <= 126) {
            if (len < max_len - 1) {
                if (pos == len) {
                    buffer[pos] = ch;
                    pos++;
                    len++;
                    buffer[len] = '\0';
                    char s[2] = {ch, '\0'};
                    print(s);
                } else {
                    for (int i = len; i > pos; i--) {
                        buffer[i] = buffer[i - 1];
                    }
                    buffer[pos] = ch;
                    pos++;
                    len++;
                    buffer[len] = '\0';
                    for (int i = pos - 1; i < len; i++) {
                        char s[2] = {buffer[i], '\0'};
                        print(s);
                    }
                    for (int i = len; i > pos; i--) {
                        print("\b");
                    }
                }
            }
        }
    }
}

char* readline(const char* prompt) {
    static char s_readline_buf[256];
    int len = readline_buf(s_readline_buf, sizeof(s_readline_buf), prompt);
    if (len < 0) {
        return NULL;
    }
    char* dynamic_str = (char*)kmalloc(len + 1);
    if (dynamic_str) {
        strcpy(dynamic_str, s_readline_buf);
        return dynamic_str;
    }
    return s_readline_buf;
}

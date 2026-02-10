#include "calculator.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

static calc_state_t calc_states[MAX_WINDOWS];
static uint32 calc_count = 0;

static calc_state_t* get_calc_state(window_t* win) {
    if (calc_count < MAX_WINDOWS && win->app_data == NULL) {
        calc_state_t* state = &calc_states[calc_count++];
        memset(state, 0, sizeof(calc_state_t));
        state->display[0] = '0';
        state->new_number = 1;
        win->app_data = state;
        return state;
    }
    return (calc_state_t*)win->app_data;
}

static void int_to_str(sint32 num, char* buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int neg = 0;
    if (num < 0) {
        neg = 1;
        num = -num;
    }
    
    char tmp[16];
    int i = 0;
    while (num > 0 && i < 15) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void calculator_draw(window_t* win) {
    if (!win) return;
    
    calc_state_t* state = get_calc_state(win);
    if (!state) return;
    
    volatile char* video = (volatile char*)0xB8000;
    
    uint32 content_y = win->y + 1;
    uint32 content_x = win->x + 1;
    uint32 content_width = win->width - 2;
    uint32 content_height = win->height - 2;
    
    // Clear content
    for (uint32 row = 0; row < content_height; row++) {
        for (uint32 col = 0; col < content_width; col++) {
            uint32 sy = content_y + row;
            uint32 sx = content_x + col;
            if (sy < 25 && sx < 80) {
                int idx = (sy * 80 + sx) * 2;
                video[idx] = ' ';
                video[idx + 1] = (COLOR_BLACK << 4) | COLOR_WHITE;
            }
        }
    }
    
    // Display area (top)
    for (uint32 col = 0; col < content_width; col++) {
        int idx = (content_y * 80 + content_x + col) * 2;
        video[idx] = ' ';
        video[idx + 1] = (COLOR_DARK_GRAY << 4) | COLOR_WHITE;
    }
    
    // Draw display value (right aligned)
    int len = strlen(state->display);
    int start = content_width - len - 1;
    if (start < 0) start = 0;
    for (int i = 0; state->display[i] && start + i < (int)content_width; i++) {
        int idx = (content_y * 80 + content_x + start + i) * 2;
        video[idx] = state->display[i];
        video[idx + 1] = (COLOR_DARK_GRAY << 4) | COLOR_LIGHT_GREEN;
    }
    
    // Draw keypad help
    draw_text(content_x, content_y + 2, "Keys: 0-9 +-*/", COLOR_LIGHT_GRAY);
    draw_text(content_x, content_y + 3, "Enter = Calculate", COLOR_LIGHT_GRAY);
    draw_text(content_x, content_y + 4, "C = Clear", COLOR_LIGHT_GRAY);
}

void calculator_handle_key(window_t* win, char key) {
    if (!win) return;
    
    calc_state_t* state = get_calc_state(win);
    if (!state) return;
    
    if (key >= '0' && key <= '9') {
        if (state->new_number) {
            state->display[0] = key;
            state->display[1] = '\0';
            state->new_number = 0;
        } else {
            int len = strlen(state->display);
            if (len < 10) {
                state->display[len] = key;
                state->display[len + 1] = '\0';
            }
        }
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        // Store first value and operator
        state->value1 = 0;
        int neg = 0;
        int i = 0;
        if (state->display[0] == '-') {
            neg = 1;
            i = 1;
        }
        for (; state->display[i]; i++) {
            state->value1 = state->value1 * 10 + (state->display[i] - '0');
        }
        if (neg) state->value1 = -state->value1;
        
        state->operator = key;
        state->has_operator = 1;
        state->new_number = 1;
    } else if (key == '\n' || key == '=') {
        if (state->has_operator) {
            // Parse second value
            state->value2 = 0;
            int neg = 0;
            int i = 0;
            if (state->display[0] == '-') {
                neg = 1;
                i = 1;
            }
            for (; state->display[i]; i++) {
                state->value2 = state->value2 * 10 + (state->display[i] - '0');
            }
            if (neg) state->value2 = -state->value2;
            
            // Calculate
            switch (state->operator) {
                case '+': state->result = state->value1 + state->value2; break;
                case '-': state->result = state->value1 - state->value2; break;
                case '*': state->result = state->value1 * state->value2; break;
                case '/': 
                    if (state->value2 != 0) 
                        state->result = state->value1 / state->value2;
                    else
                        state->result = 0;
                    break;
            }
            
            int_to_str(state->result, state->display);
            state->has_operator = 0;
            state->new_number = 1;
        }
    } else if (key == 'c' || key == 'C') {
        memset(state, 0, sizeof(calc_state_t));
        state->display[0] = '0';
        state->new_number = 1;
    }
    
    calculator_draw(win);
}

window_t* calculator_create(void) {
    window_t* win = desktop_create_window("Calculator", 55, 5, 22, 8);
    if (!win) return NULL;
    
    win->draw_content = calculator_draw;
    win->handle_key = calculator_handle_key;
    win->app_data = NULL;
    
    get_calc_state(win);
    
    return win;
}

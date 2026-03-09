#include "calculator.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

#define CALC_BUTTON_W 34
#define CALC_BUTTON_H 18
#define CALC_BUTTON_GAP 4

static calc_state_t g_calc_states[MAX_WINDOWS];
static uint32 g_calc_count = 0;
static const char* g_calc_rows[4][4] = {
    {"7", "8", "9", "/"},
    {"4", "5", "6", "*"},
    {"1", "2", "3", "-"},
    {"C", "0", "=", "+"}
};

static calc_state_t* get_calc_state(window_t* win) {
    if (!win) {
        return NULL;
    }

    if (win->app_data) {
        return (calc_state_t*)win->app_data;
    }

    if (g_calc_count >= MAX_WINDOWS) {
        return NULL;
    }

    calc_state_t* state = &g_calc_states[g_calc_count++];
    memset(state, 0, sizeof(calc_state_t));
    state->display[0] = '0';
    state->new_number = 1;
    win->app_data = state;
    return state;
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

    int pos = 0;
    if (neg) {
        buf[pos++] = '-';
    }
    while (i > 0) {
        buf[pos++] = tmp[--i];
    }
    buf[pos] = '\0';
}

static sint32 parse_display(const char* display) {
    sint32 value = 0;
    sint32 sign = 1;
    uint32 i = 0;

    if (display[0] == '-') {
        sign = -1;
        i = 1;
    }

    for (; display[i]; i++) {
        value = value * 10 + (display[i] - '0');
    }

    return value * sign;
}

static void calculator_apply_input(window_t* win, calc_state_t* state, char key) {
    if (!win || !state) {
        return;
    }

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
        return;
    }

    if (key == '+' || key == '-' || key == '*' || key == '/') {
        state->value1 = parse_display(state->display);
        state->operator = key;
        state->has_operator = 1;
        state->new_number = 1;
        return;
    }

    if (key == '\n' || key == '=') {
        if (state->has_operator) {
            state->value2 = parse_display(state->display);
            switch (state->operator) {
                case '+': state->result = state->value1 + state->value2; break;
                case '-': state->result = state->value1 - state->value2; break;
                case '*': state->result = state->value1 * state->value2; break;
                case '/': state->result = (state->value2 != 0) ? (state->value1 / state->value2) : 0; break;
                default: state->result = 0; break;
            }

            int_to_str(state->result, state->display);
            state->has_operator = 0;
            state->new_number = 1;
        }
        return;
    }

    if (key == 'c' || key == 'C') {
        memset(state, 0, sizeof(calc_state_t));
        state->display[0] = '0';
        state->new_number = 1;
    }
}

void calculator_draw(window_t* win) {
    calc_state_t* state = get_calc_state(win);
    rect_t content;
    if (!win || !state) {
        return;
    }

    desktop_get_window_content_rect(win, &content);

    draw_filled_rect((uint32)content.x + 4, (uint32)content.y + 8, (uint32)content.width - 8, 26, COLOR_BLACK);
    draw_rect((uint32)content.x + 4, (uint32)content.y + 8, (uint32)content.width - 8, 26, COLOR_LIGHT_CYAN);
    draw_text((uint32)content.x + 10, (uint32)content.y + 17, state->display, COLOR_LIGHT_GREEN);
    draw_text((uint32)content.x + 118, (uint32)content.y + 17, state->has_operator ? "op" : "in", COLOR_DARK_GRAY);

    for (uint32 row = 0; row < 4; row++) {
        for (uint32 col = 0; col < 4; col++) {
            uint32 x = (uint32)content.x + 6 + col * (CALC_BUTTON_W + CALC_BUTTON_GAP);
            uint32 y = (uint32)content.y + 44 + row * (CALC_BUTTON_H + CALC_BUTTON_GAP);
            const char* label = g_calc_rows[row][col];
            uint8 fill = (label[0] >= '0' && label[0] <= '9') ? COLOR_LIGHT_GRAY : COLOR_CYAN;
            if (label[0] == '=') {
                fill = COLOR_GREEN;
            } else if (label[0] == 'C') {
                fill = COLOR_RED;
            }
            if (state->last_button[0] == label[0] && label[1] == '\0') {
                fill = COLOR_LIGHT_CYAN;
            }

            draw_filled_rect(x, y, CALC_BUTTON_W, CALC_BUTTON_H, fill);
            draw_rect(x, y, CALC_BUTTON_W, CALC_BUTTON_H, COLOR_WHITE);
            draw_text(x + 13, y + 6, label, COLOR_BLACK);
        }
    }
}

void calculator_handle_key(window_t* win, char key) {
    calc_state_t* state = get_calc_state(win);
    if (!win || !state) {
        return;
    }

    state->last_button[0] = key;
    state->last_button[1] = '\0';
    calculator_apply_input(win, state, key);
}

void calculator_handle_mouse(window_t* win, sint32 local_x, sint32 local_y, uint8 buttons) {
    calc_state_t* state = get_calc_state(win);
    if (!win || !state || !(buttons & 0x01)) {
        return;
    }

    if (local_y < 44 || local_x < 6) {
        return;
    }

    sint32 grid_x = local_x - 6;
    sint32 grid_y = local_y - 44;
    sint32 col = grid_x / (CALC_BUTTON_W + CALC_BUTTON_GAP);
    sint32 row = grid_y / (CALC_BUTTON_H + CALC_BUTTON_GAP);

    if (col < 0 || col >= 4 || row < 0 || row >= 4) {
        return;
    }

    if ((grid_x % (CALC_BUTTON_W + CALC_BUTTON_GAP)) >= CALC_BUTTON_W) {
        return;
    }
    if ((grid_y % (CALC_BUTTON_H + CALC_BUTTON_GAP)) >= CALC_BUTTON_H) {
        return;
    }

    state->last_button[0] = g_calc_rows[row][col][0];
    state->last_button[1] = '\0';
    calculator_apply_input(win, state, g_calc_rows[row][col][0]);
}

window_t* calculator_create(void) {
    window_t* win = desktop_create_window("Calculator", 84, 24, 188, 132);
    if (!win) {
        return NULL;
    }

    win->min_width = 160;
    win->min_height = 118;
    win->draw_content = calculator_draw;
    win->handle_key = calculator_handle_key;
    win->handle_mouse = calculator_handle_mouse;
    get_calc_state(win);
    return win;
}

#include "calculator.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

static calc_state_t g_calc_states[MAX_WINDOWS];
static uint32 g_calc_count = 0;

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

void calculator_draw(window_t* win) {
    calc_state_t* state = get_calc_state(win);
    if (!win || !state) {
        return;
    }

    draw_filled_rect(win->x + 2, win->y + 20, win->width - 4, win->height - 22, COLOR_WHITE);
    draw_filled_rect(win->x + 12, win->y + 30, win->width - 24, 26, COLOR_BLACK);
    draw_rect(win->x + 12, win->y + 30, win->width - 24, 26, COLOR_LIGHT_CYAN);
    draw_text(win->x + 18, win->y + 39, state->display, COLOR_LIGHT_GREEN);

    draw_text(win->x + 14, win->y + 68, "Keys 0-9 and +-*/", COLOR_BLACK);
    draw_text(win->x + 14, win->y + 82, "Enter or = to solve", COLOR_BLACK);
    draw_text(win->x + 14, win->y + 96, "C clears", COLOR_BLACK);
}

void calculator_handle_key(window_t* win, char key) {
    calc_state_t* state = get_calc_state(win);
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

window_t* calculator_create(void) {
    window_t* win = desktop_create_window("Calculator", 86, 32, 170, 120);
    if (!win) {
        return NULL;
    }

    win->draw_content = calculator_draw;
    win->handle_key = calculator_handle_key;
    get_calc_state(win);
    return win;
}

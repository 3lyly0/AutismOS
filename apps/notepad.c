#include "notepad.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

#define CONTENT_PAD 8
#define CHAR_W 6
#define CHAR_H 8

static notepad_state_t g_notepad_states[MAX_WINDOWS];
static uint32 g_notepad_count = 0;

static notepad_state_t* get_notepad_state(window_t* win) {
    if (!win) {
        return NULL;
    }

    if (win->app_data) {
        return (notepad_state_t*)win->app_data;
    }

    if (g_notepad_count >= MAX_WINDOWS) {
        return NULL;
    }

    notepad_state_t* state = &g_notepad_states[g_notepad_count++];
    memset(state, 0, sizeof(notepad_state_t));
    win->app_data = state;
    return state;
}

void notepad_draw(window_t* win) {
    notepad_state_t* state = get_notepad_state(win);
    if (!win || !state) {
        return;
    }

    uint32 content_x = win->x + CONTENT_PAD;
    uint32 content_y = win->y + 26;
    uint32 content_w = win->width - (CONTENT_PAD * 2);
    uint32 content_h = win->height - 34;
    uint32 cols = content_w / CHAR_W;
    uint32 rows = content_h / CHAR_H;

    draw_filled_rect(win->x + 2, win->y + 20, win->width - 4, win->height - 22, COLOR_WHITE);
    draw_text(content_x, win->y + 8, "Type here. Backspace works.", COLOR_WHITE);

    uint32 row = 0;
    uint32 col = 0;
    for (uint32 i = 0; i < state->text_len && row < rows; i++) {
        char ch = state->text[i];
        if (ch == '\n' || col >= cols) {
            row++;
            col = 0;
            if (ch == '\n') {
                continue;
            }
        }

        draw_char(content_x + col * CHAR_W, content_y + row * CHAR_H, ch, COLOR_BLACK);
        col++;
    }

    row = 0;
    col = 0;
    for (uint32 i = 0; i < state->cursor_pos && i < state->text_len; i++) {
        if (state->text[i] == '\n' || col >= cols) {
            row++;
            col = 0;
            if (state->text[i] != '\n') {
                col++;
            }
        } else {
            col++;
        }
    }

    if (row < rows) {
        draw_filled_rect(content_x + col * CHAR_W, content_y + row * CHAR_H, 2, 7, COLOR_BLUE);
    }
}

void notepad_handle_key(window_t* win, char key) {
    notepad_state_t* state = get_notepad_state(win);
    if (!win || !state) {
        return;
    }

    if (key == '\b') {
        if (state->cursor_pos > 0 && state->text_len > 0) {
            for (uint32 i = state->cursor_pos - 1; i + 1 < state->text_len; i++) {
                state->text[i] = state->text[i + 1];
            }
            state->text_len--;
            state->cursor_pos--;
            state->text[state->text_len] = '\0';
        }
    } else if (key == '\n') {
        if (state->text_len < NOTEPAD_MAX_TEXT - 1) {
            state->text[state->cursor_pos++] = '\n';
            state->text_len++;
            state->text[state->text_len] = '\0';
        }
    } else if (key >= 32 && key <= 126) {
        if (state->text_len < NOTEPAD_MAX_TEXT - 1) {
            for (uint32 i = state->text_len; i > state->cursor_pos; i--) {
                state->text[i] = state->text[i - 1];
            }
            state->text[state->cursor_pos++] = key;
            state->text_len++;
            state->text[state->text_len] = '\0';
        }
    }
}

window_t* notepad_create(void) {
    window_t* win = desktop_create_window("Notepad", 22, 14, 220, 150);
    if (!win) {
        return NULL;
    }

    win->draw_content = notepad_draw;
    win->handle_key = notepad_handle_key;
    get_notepad_state(win);
    return win;
}

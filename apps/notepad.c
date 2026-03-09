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
    rect_t content;
    const desktop_chrome_t* chrome = desktop_get_chrome();
    if (!win || !state) {
        return;
    }

    desktop_get_window_content_rect(win, &content);

    uint32 content_x = (uint32)content.x + CONTENT_PAD;
    uint32 content_y = (uint32)content.y + 12;
    uint32 content_w = (uint32)content.width - (CONTENT_PAD * 2);
    uint32 content_h = (uint32)content.height - 18;
    uint32 cols = content_w / CHAR_W;
    uint32 rows = content_h / CHAR_H;

    draw_text(win->x + chrome->content_padding, win->y + 5, "Type here. Backspace works.", COLOR_WHITE);
    draw_line(content.x, content.y + 8, content.x + content.width - 1, content.y + 8, COLOR_LIGHT_GRAY);

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

    win->min_width = 160;
    win->min_height = 100;
    win->draw_content = notepad_draw;
    win->handle_key = notepad_handle_key;
    get_notepad_state(win);
    return win;
}

#include "notepad.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "memory.h"

static notepad_state_t notepad_states[MAX_WINDOWS];
static uint32 notepad_count = 0;

static notepad_state_t* get_notepad_state(window_t* win) {
    // Use window id to find state (simple approach)
    for (uint32 i = 0; i < notepad_count; i++) {
        if (i == win->id - 1 && i < MAX_WINDOWS) {
            return &notepad_states[i];
        }
    }
    // Create new state
    if (notepad_count < MAX_WINDOWS) {
        notepad_state_t* state = &notepad_states[notepad_count++];
        memset(state, 0, sizeof(notepad_state_t));
        return state;
    }
    return NULL;
}

void notepad_draw(window_t* win) {
    if (!win) return;
    
    notepad_state_t* state = get_notepad_state(win);
    if (!state) return;
    
    volatile char* video = (volatile char*)0xB8000;
    
    // Content area starts after title bar
    uint32 content_y = win->y + 1;
    uint32 content_x = win->x + 1;
    uint32 content_width = win->width - 2;
    uint32 content_height = win->height - 2;
    
    // Clear content area
    uint8 bg_color = COLOR_LIGHT_GRAY;
    uint8 text_color = COLOR_BLACK;
    
    for (uint32 row = 0; row < content_height; row++) {
        for (uint32 col = 0; col < content_width; col++) {
            uint32 screen_row = content_y + row;
            uint32 screen_col = content_x + col;
            if (screen_row < 25 && screen_col < 80) {
                int index = (screen_row * 80 + screen_col) * 2;
                video[index] = ' ';
                video[index + 1] = (bg_color << 4) | text_color;
            }
        }
    }
    
    // Draw text with word wrap
    uint32 char_idx = 0;
    uint32 row = 0;
    uint32 col = 0;
    
    while (char_idx < state->text_len && row < content_height) {
        char c = state->text[char_idx];
        
        if (c == '\n' || col >= content_width) {
            row++;
            col = 0;
            if (c == '\n') char_idx++;
            continue;
        }
        
        uint32 screen_row = content_y + row;
        uint32 screen_col = content_x + col;
        
        if (screen_row < 25 && screen_col < 80) {
            int index = (screen_row * 80 + screen_col) * 2;
            video[index] = c;
            video[index + 1] = (bg_color << 4) | text_color;
        }
        
        char_idx++;
        col++;
    }
    
    // Draw cursor
    col = 0;
    row = 0;
    for (uint32 i = 0; i < state->cursor_pos && i < state->text_len; i++) {
        if (state->text[i] == '\n' || col >= content_width) {
            row++;
            col = 0;
            if (state->text[i] != '\n') {
                col++;
            }
        } else {
            col++;
        }
    }
    
    uint32 cursor_screen_y = content_y + row;
    uint32 cursor_screen_x = content_x + col;
    
    if (cursor_screen_y < 25 && cursor_screen_x < 80) {
        int index = (cursor_screen_y * 80 + cursor_screen_x) * 2;
        video[index + 1] = (COLOR_BLACK << 4) | COLOR_WHITE;  // Inverted colors for cursor
    }
}

void notepad_handle_key(window_t* win, char key) {
    if (!win) return;
    
    notepad_state_t* state = get_notepad_state(win);
    if (!state) return;
    
    if (key == '\b') {  // Backspace
        if (state->cursor_pos > 0 && state->text_len > 0) {
            // Shift text left
            for (uint32 i = state->cursor_pos - 1; i < state->text_len - 1; i++) {
                state->text[i] = state->text[i + 1];
            }
            state->text_len--;
            state->cursor_pos--;
            state->text[state->text_len] = '\0';
        }
    } else if (key == 0x1B) {  // Escape - do nothing or close
        // Could add close functionality
    } else if (key >= 32 || key == '\n') {  // Printable or newline
        if (state->text_len < NOTEPAD_MAX_TEXT - 1) {
            // Insert at cursor
            for (uint32 i = state->text_len; i > state->cursor_pos; i--) {
                state->text[i] = state->text[i - 1];
            }
            state->text[state->cursor_pos] = key;
            state->cursor_pos++;
            state->text_len++;
            state->text[state->text_len] = '\0';
        }
    }
    
    // Redraw notepad
    notepad_draw(win);
}

window_t* notepad_create(void) {
    window_t* win = desktop_create_window("Notepad", 5, 2, 50, 15);
    if (!win) return NULL;
    
    win->draw_content = notepad_draw;
    win->handle_key = notepad_handle_key;
    
    // Initialize state
    get_notepad_state(win);
    
    return win;
}

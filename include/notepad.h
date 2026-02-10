#ifndef NOTEPAD_H
#define NOTEPAD_H

#include "types.h"
#include "desktop.h"

#define NOTEPAD_MAX_TEXT 1024
#define NOTEPAD_MAX_LINES 20
#define NOTEPAD_LINE_WIDTH 60

typedef struct {
    char text[NOTEPAD_MAX_TEXT];
    uint32 text_len;
    uint32 cursor_pos;
    uint32 scroll_offset;
} notepad_state_t;

// Initialize and launch notepad
window_t* notepad_create(void);

// Notepad callbacks
void notepad_draw(window_t* win);
void notepad_handle_key(window_t* win, char key);

#endif

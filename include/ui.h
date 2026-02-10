#ifndef UI_H
#define UI_H

#include "types.h"

// UI Components for Step 9

// Maximum textbox buffer size
#define TEXTBOX_MAX_LEN 256

// Textbox structure
typedef struct textbox {
    uint32 x;           // X position on screen
    uint32 y;           // Y position on screen
    uint32 w;           // Width in characters
    uint32 h;           // Height in characters (usually 1)
    char buffer[TEXTBOX_MAX_LEN];  // Input buffer
    uint32 cursor_pos;  // Cursor position in buffer
    uint8 focused;      // Is this textbox focused?
    uint8 border_color; // Border color
    uint8 text_color;   // Text color
} textbox_t;

// Focus management
typedef enum {
    FOCUS_NONE = 0,
    FOCUS_TEXTBOX = 1
} focus_type_t;

// Global focus state
typedef struct {
    focus_type_t type;
    void* element;      // Pointer to focused element
    uint8 caret_visible; // Caret blink state
} focus_state_t;

// Initialize UI subsystem
void ui_init(void);

// Textbox functions
void textbox_init(textbox_t* box, uint32 x, uint32 y, uint32 w, uint32 h);
void textbox_render(textbox_t* box);
void textbox_handle_char(textbox_t* box, char ch);
void textbox_handle_backspace(textbox_t* box);
void textbox_clear(textbox_t* box);
const char* textbox_get_text(textbox_t* box);

// Focus management
void ui_set_focus(focus_type_t type, void* element);
void* ui_get_focused_element(void);
focus_type_t ui_get_focus_type(void);

// Caret blink (called from timer interrupt)
void ui_update_caret_blink(void);
void ui_render_caret(void);

#endif

#include "ui.h"
#include "graphics.h"
#include "string.h"
#include <stddef.h>

// Global focus state
static focus_state_t g_focus_state = {FOCUS_NONE, NULL, 0};

// Caret blink counter (updated by timer)
static uint32 g_caret_blink_ticks = 0;
#define CARET_BLINK_RATE 25  // ~250ms at 100Hz timer

// Initialize UI subsystem
void ui_init(void) {
    g_focus_state.type = FOCUS_NONE;
    g_focus_state.element = NULL;
    g_focus_state.caret_visible = 1;
    g_caret_blink_ticks = 0;
}

// Initialize a textbox
void textbox_init(textbox_t* box, uint32 x, uint32 y, uint32 w, uint32 h) {
    if (!box) return;
    
    box->x = x;
    box->y = y;
    box->w = w;
    box->h = h;
    box->cursor_pos = 0;
    box->focused = 0;
    box->border_color = COLOR_LIGHT_GRAY;
    box->text_color = COLOR_WHITE;
    memset(box->buffer, 0, TEXTBOX_MAX_LEN);
}

// Render a textbox
void textbox_render(textbox_t* box) {
    if (!box) return;
    
    // Clear the textbox area
    graphics_clear_region(box->x, box->y, box->w, box->h, COLOR_BLACK);
    
    // Draw border (different color if focused)
    uint8 border_color = box->focused ? COLOR_LIGHT_CYAN : COLOR_LIGHT_GRAY;
    draw_rect(box->x, box->y, box->w, box->h, border_color);
    
    // Draw text content
    if (box->buffer[0]) {
        // Calculate visible portion of text
        uint32 text_x = box->x + 2;
        uint32 text_y = box->y + 1;
        uint32 visible_chars = box->w - 4;  // Leave space for borders
        
        // Draw visible portion of text
        uint32 start_offset = 0;
        if (box->cursor_pos >= visible_chars) {
            start_offset = box->cursor_pos - visible_chars + 1;
        }
        
        uint32 i = 0;
        for (uint32 pos = start_offset; pos < TEXTBOX_MAX_LEN && box->buffer[pos] && i < visible_chars; pos++, i++) {
            draw_char(text_x + i, text_y, box->buffer[pos], box->text_color);
        }
    }
}

// Handle character input
void textbox_handle_char(textbox_t* box, char ch) {
    if (!box || !box->focused) return;
    
    // Check if we have space
    if (box->cursor_pos >= TEXTBOX_MAX_LEN - 1) {
        return;
    }
    
    // Add character at cursor position
    box->buffer[box->cursor_pos] = ch;
    box->cursor_pos++;
    box->buffer[box->cursor_pos] = '\0';
    
    // Redraw textbox
    textbox_render(box);
}

// Handle backspace
void textbox_handle_backspace(textbox_t* box) {
    if (!box || !box->focused) return;
    
    if (box->cursor_pos > 0) {
        box->cursor_pos--;
        box->buffer[box->cursor_pos] = '\0';
        
        // Redraw textbox
        textbox_render(box);
    }
}

// Clear textbox
void textbox_clear(textbox_t* box) {
    if (!box) return;
    
    memset(box->buffer, 0, TEXTBOX_MAX_LEN);
    box->cursor_pos = 0;
    textbox_render(box);
}

// Get textbox text
const char* textbox_get_text(textbox_t* box) {
    if (!box) return NULL;
    return box->buffer;
}

// Set focus to an element
void ui_set_focus(focus_type_t type, void* element) {
    // Unfocus previous element
    if (g_focus_state.type == FOCUS_TEXTBOX && g_focus_state.element) {
        textbox_t* prev_box = (textbox_t*)g_focus_state.element;
        prev_box->focused = 0;
        textbox_render(prev_box);
    }
    
    // Set new focus
    g_focus_state.type = type;
    g_focus_state.element = element;
    
    // Focus new element
    if (type == FOCUS_TEXTBOX && element) {
        textbox_t* box = (textbox_t*)element;
        box->focused = 1;
        textbox_render(box);
    }
}

// Get focused element
void* ui_get_focused_element(void) {
    return g_focus_state.element;
}

// Get focus type
focus_type_t ui_get_focus_type(void) {
    return g_focus_state.type;
}

// Update caret blink state (called from timer interrupt)
void ui_update_caret_blink(void) {
    g_caret_blink_ticks++;
    
    if (g_caret_blink_ticks >= CARET_BLINK_RATE) {
        g_caret_blink_ticks = 0;
        g_focus_state.caret_visible = !g_focus_state.caret_visible;
    }
}

// Render caret for focused element
void ui_render_caret(void) {
    if (g_focus_state.type != FOCUS_TEXTBOX || !g_focus_state.element) {
        return;
    }
    
    textbox_t* box = (textbox_t*)g_focus_state.element;
    if (!box->focused) {
        return;
    }
    
    // Calculate caret position
    uint32 text_x = box->x + 2;
    uint32 text_y = box->y + 1;
    uint32 visible_chars = box->w - 4;
    
    uint32 caret_offset = box->cursor_pos;
    if (box->cursor_pos >= visible_chars) {
        caret_offset = visible_chars - 1;
    }
    
    uint32 caret_x = text_x + caret_offset;
    uint32 caret_y = text_y;
    
    // Draw or hide caret based on blink state
    draw_cursor(caret_x, caret_y, g_focus_state.caret_visible);
}

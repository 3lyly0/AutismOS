#include "widget.h"
#include "graphics.h"
#include "string.h"
#include "kheap.h"

/*
 * Widget System Implementation - AutismOS
 */

// ============================================================================
// Base Widget Functions
// ============================================================================

void widget_init(widget_t* w, widget_type_t type, const widget_class_t* vtable) {
    if (!w) return;
    
    memset(w, 0, sizeof(widget_t));
    w->type = type;
    w->vtable = vtable;
    w->visible = 1;
    w->enabled = 1;
    w->needs_redraw = 1;
    
    // Default colors
    w->bg_color = COLOR_LIGHT_GRAY;
    w->fg_color = COLOR_BLACK;
    w->border_color = COLOR_DARK_GRAY;
}

void widget_set_position(widget_t* w, sint32 x, sint32 y) {
    if (!w) return;
    w->x = x;
    w->y = y;
    w->needs_redraw = 1;
}

void widget_set_size(widget_t* w, uint32 width, uint32 height) {
    if (!w) return;
    w->width = width;
    w->height = height;
    w->needs_redraw = 1;
}

void widget_set_colors(widget_t* w, uint8 bg, uint8 fg, uint8 border) {
    if (!w) return;
    w->bg_color = bg;
    w->fg_color = fg;
    w->border_color = border;
    w->needs_redraw = 1;
}

void widget_set_visible(widget_t* w, uint8 visible) {
    if (!w) return;
    w->visible = visible;
    w->needs_redraw = 1;
}

void widget_set_enabled(widget_t* w, uint8 enabled) {
    if (!w) return;
    w->enabled = enabled;
    w->state = enabled ? WIDGET_STATE_NORMAL : WIDGET_STATE_DISABLED;
    w->needs_redraw = 1;
}

void widget_add_child(widget_t* parent, widget_t* child) {
    if (!parent || !child) return;
    
    // Simple implementation - no dynamic allocation for now
    // In future, use kheap to allocate children array
    child->parent = parent;
}

void widget_remove_child(widget_t* parent, widget_t* child) {
    if (!parent || !child) return;
    child->parent = NULL;
}

void widget_draw(widget_t* w) {
    if (!w || !w->visible) return;
    if (!w->vtable || !w->vtable->draw) return;
    w->vtable->draw(w);
    w->needs_redraw = 0;
}

void widget_handle_event(widget_t* w, const input_event_t* event) {
    if (!w || !w->enabled || !w->visible) return;
    if (!w->vtable || !w->vtable->handle_event) return;
    w->vtable->handle_event(w, event);
}

void widget_destroy(widget_t* w) {
    if (!w) return;
    if (w->vtable && w->vtable->destroy) {
        w->vtable->destroy(w);
    }
}

uint8 widget_contains_point(widget_t* w, sint32 x, sint32 y) {
    if (!w) return 0;
    return x >= w->x && x < (sint32)(w->x + w->width) &&
           y >= w->y && y < (sint32)(w->y + w->height);
}

widget_t* widget_find_at(widget_t* root, sint32 x, sint32 y) {
    if (!root || !widget_contains_point(root, x, y)) return NULL;
    return root;  // Simple version - can be extended for children
}

// ============================================================================
// Label Widget
// ============================================================================

static void label_draw(widget_t* w) {
    label_t* label = (label_t*)w;
    if (!label || !label->text) return;
    
    // Calculate text position based on alignment
    uint32 text_len = strlen(label->text);
    uint32 text_x = label->base.x;
    uint32 text_y = label->base.y;
    
    // Horizontal alignment
    if (label->halign == ALIGN_CENTER) {
        text_x = label->base.x + (label->base.width - text_len * 8) / 2;
    } else if (label->halign == ALIGN_RIGHT) {
        text_x = label->base.x + label->base.width - text_len * 8;
    }
    
    // Vertical alignment
    if (label->valign == ALIGN_MIDDLE) {
        text_y = label->base.y + (label->base.height - 8) / 2;
    } else if (label->valign == ALIGN_BOTTOM) {
        text_y = label->base.y + label->base.height - 8;
    }
    
    draw_text(text_x, text_y, label->text, label->text_color);
}

static const widget_class_t label_class = {
    .draw = label_draw,
    .handle_event = NULL,
    .destroy = NULL
};

void label_init(label_t* label, const char* text) {
    if (!label) return;
    
    widget_init(&label->base, WIDGET_LABEL, &label_class);
    label->text = text;
    label->text_color = COLOR_WHITE;
    label->halign = ALIGN_LEFT;
    label->valign = ALIGN_TOP;
    
    // Auto-size based on text
    if (text) {
        label->base.width = strlen(text) * 8;
        label->base.height = 8;
    }
}

void label_set_text(label_t* label, const char* text) {
    if (!label) return;
    label->text = text;
    label->base.needs_redraw = 1;
}

// ============================================================================
// Button Widget
// ============================================================================

static void button_draw(widget_t* w) {
    button_t* btn = (button_t*)w;
    if (!btn) return;
    
    uint8 bg = btn->base.bg_color;
    uint8 border = btn->base.border_color;
    
    // Adjust colors based on state
    if (btn->base.state == WIDGET_STATE_DISABLED) {
        bg = COLOR_DARK_GRAY;
        border = COLOR_DARK_GRAY;
    } else if (btn->base.state == WIDGET_STATE_PRESSED || btn->pressed) {
        bg = COLOR_DARK_GRAY;
        border = COLOR_WHITE;
    } else if (btn->base.state == WIDGET_STATE_HOVERED) {
        bg = COLOR_LIGHT_CYAN;
        border = COLOR_BLUE;
    } else if (btn->base.state == WIDGET_STATE_FOCUSED) {
        border = COLOR_LIGHT_CYAN;
    }
    
    // Draw button background
    draw_filled_rect(btn->base.x, btn->base.y, btn->base.width, btn->base.height, bg);
    
    // Draw border
    draw_rect(btn->base.x, btn->base.y, btn->base.width, btn->base.height, border);
    
    // Draw text centered
    if (btn->text) {
        uint32 text_len = strlen(btn->text);
        uint32 text_x = btn->base.x + (btn->base.width - text_len * 8) / 2;
        uint32 text_y = btn->base.y + (btn->base.height - 8) / 2;
        draw_text(text_x, text_y, btn->text, btn->base.fg_color);
    }
}

static void button_handle_event(widget_t* w, const input_event_t* event) {
    button_t* btn = (button_t*)w;
    if (!btn || !btn->base.enabled) return;
    
    if (event->type == INPUT_EVENT_MOUSE_MOVE) {
        sint32 mx = event->data.mouse.x;
        sint32 my = event->data.mouse.y;
        
        if (widget_contains_point(w, mx, my)) {
            if (btn->base.state != WIDGET_STATE_HOVERED && !btn->pressed) {
                btn->base.state = WIDGET_STATE_HOVERED;
                btn->base.needs_redraw = 1;
            }
        } else {
            if (btn->base.state == WIDGET_STATE_HOVERED) {
                btn->base.state = WIDGET_STATE_NORMAL;
                btn->base.needs_redraw = 1;
            }
        }
    }
    
    if (event->type == INPUT_EVENT_MOUSE_PRESS) {
        if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
            sint32 mx = event->data.mouse.x;
            sint32 my = event->data.mouse.y;
            
            if (widget_contains_point(w, mx, my)) {
                btn->pressed = 1;
                btn->base.state = WIDGET_STATE_PRESSED;
                btn->base.needs_redraw = 1;
            }
        }
    }
    
    if (event->type == INPUT_EVENT_MOUSE_RELEASE) {
        if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
            sint32 mx = event->data.mouse.x;
            sint32 my = event->data.mouse.y;
            
            if (btn->pressed && widget_contains_point(w, mx, my)) {
                // Button clicked!
                if (btn->on_click) {
                    btn->on_click(w, btn->click_data);
                }
            }
            
            btn->pressed = 0;
            btn->base.state = widget_contains_point(w, mx, my) ? 
                              WIDGET_STATE_HOVERED : WIDGET_STATE_NORMAL;
            btn->base.needs_redraw = 1;
        }
    }
}

static const widget_class_t button_class = {
    .draw = button_draw,
    .handle_event = button_handle_event,
    .destroy = NULL
};

void button_init(button_t* btn, const char* text) {
    if (!btn) return;
    
    widget_init(&btn->base, WIDGET_BUTTON, &button_class);
    btn->text = text;
    btn->on_click = NULL;
    btn->click_data = NULL;
    btn->pressed = 0;
    
    // Default button size
    btn->base.width = 80;
    btn->base.height = 24;
    btn->base.bg_color = COLOR_LIGHT_GRAY;
    btn->base.fg_color = COLOR_BLACK;
    btn->base.border_color = COLOR_DARK_GRAY;
    
    // Auto-size if text provided
    if (text) {
        uint32 text_width = strlen(text) * 8 + 16;
        if (text_width > btn->base.width) {
            btn->base.width = text_width;
        }
    }
}

void button_set_click_handler(button_t* btn, button_click_fn handler, void* user_data) {
    if (!btn) return;
    btn->on_click = handler;
    btn->click_data = user_data;
}

// ============================================================================
// Panel Widget
// ============================================================================

static void panel_draw(widget_t* w) {
    panel_t* panel = (panel_t*)w;
    if (!panel) return;
    
    // Fill background
    if (panel->fill_background) {
        draw_filled_rect(panel->base.x, panel->base.y, 
                        panel->base.width, panel->base.height, 
                        panel->base.bg_color);
    }
    
    // Draw border
    if (panel->draw_border) {
        draw_rect(panel->base.x, panel->base.y, 
                 panel->base.width, panel->base.height, 
                 panel->base.border_color);
    }
    
    // Draw title if present
    if (panel->title && panel->draw_border) {
        uint32 title_width = strlen(panel->title) * 8 + 8;
        draw_filled_rect(panel->base.x + 4, panel->base.y - 10, 
                        title_width, 12, panel->base.bg_color);
        draw_text(panel->base.x + 8, panel->base.y - 8, 
                 panel->title, panel->base.fg_color);
    }
}

static const widget_class_t panel_class = {
    .draw = panel_draw,
    .handle_event = NULL,
    .destroy = NULL
};

void panel_init(panel_t* panel) {
    if (!panel) return;
    
    widget_init(&panel->base, WIDGET_PANEL, &panel_class);
    panel->draw_border = 1;
    panel->fill_background = 1;
    panel->title = NULL;
    
    panel->base.bg_color = COLOR_LIGHT_GRAY;
    panel->base.border_color = COLOR_DARK_GRAY;
}

void panel_set_title(panel_t* panel, const char* title) {
    if (!panel) return;
    panel->title = title;
    panel->base.needs_redraw = 1;
}

// ============================================================================
// Textbox Widget
// ============================================================================

static void textbox_widget_draw(widget_t* w) {
    textbox_widget_t* tb = (textbox_widget_t*)w;
    if (!tb) return;
    
    // Background
    uint8 bg = tb->readonly ? COLOR_DARK_GRAY : COLOR_BLACK;
    draw_filled_rect(tb->base.x, tb->base.y, tb->base.width, tb->base.height, bg);
    
    // Border - highlight if focused
    uint8 border = (tb->base.state == WIDGET_STATE_FOCUSED) ? 
                   COLOR_LIGHT_CYAN : COLOR_DARK_GRAY;
    draw_rect(tb->base.x, tb->base.y, tb->base.width, tb->base.height, border);
    
    // Calculate visible text area
    uint32 text_x = tb->base.x + 4;
    uint32 text_y = tb->base.y + 4;
    uint32 visible_chars = (tb->base.width - 8) / 8;
    
    // Calculate scroll offset for long text
    uint32 start_offset = 0;
    if (tb->cursor_pos >= visible_chars) {
        start_offset = tb->cursor_pos - visible_chars + 1;
    }
    
    // Draw text
    uint32 i = 0;
    for (uint32 pos = start_offset; pos < WIDGET_TEXTBOX_MAX_LEN && 
         tb->buffer[pos] && i < visible_chars; pos++, i++) {
        char ch = tb->buffer[pos];
        if (tb->password_mode) {
            ch = '*';
        }
        draw_char(text_x + i * 8, text_y, ch, tb->base.fg_color);
    }
    
    // Draw cursor if focused
    if (tb->base.state == WIDGET_STATE_FOCUSED && tb->cursor_visible) {
        uint32 cursor_offset = tb->cursor_pos - start_offset;
        draw_cursor(text_x + cursor_offset * 8, text_y, 1);
    }
}

static void textbox_widget_handle_event(widget_t* w, const input_event_t* event) {
    textbox_widget_t* tb = (textbox_widget_t*)w;
    if (!tb || tb->readonly) return;
    
    if (event->type == INPUT_EVENT_MOUSE_PRESS) {
        if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
            if (widget_contains_point(w, event->data.mouse.x, event->data.mouse.y)) {
                tb->base.state = WIDGET_STATE_FOCUSED;
                tb->base.needs_redraw = 1;
            } else {
                tb->base.state = WIDGET_STATE_NORMAL;
                tb->base.needs_redraw = 1;
            }
        }
    }
    
    if (event->type == INPUT_EVENT_KEY_PRESS && 
        tb->base.state == WIDGET_STATE_FOCUSED) {
        char ch = event->data.key.character;
        keycode_t key = event->data.key.keycode;
        
        if (key == KEY_BACKSPACE) {
            if (tb->cursor_pos > 0) {
                tb->cursor_pos--;
                tb->buffer[tb->cursor_pos] = '\0';
                if (tb->on_change) tb->on_change(w, tb->buffer);
            }
        } else if (key == KEY_ENTER) {
            if (tb->on_submit) tb->on_submit(w, tb->buffer);
        } else if (ch >= 32 && ch <= 126) {
            if (tb->cursor_pos < WIDGET_TEXTBOX_MAX_LEN - 1) {
                tb->buffer[tb->cursor_pos] = ch;
                tb->cursor_pos++;
                tb->buffer[tb->cursor_pos] = '\0';
                if (tb->on_change) tb->on_change(w, tb->buffer);
            }
        }
        
        tb->base.needs_redraw = 1;
    }
}

static const widget_class_t textbox_widget_class = {
    .draw = textbox_widget_draw,
    .handle_event = textbox_widget_handle_event,
    .destroy = NULL
};

void textbox_widget_init(textbox_widget_t* tb) {
    if (!tb) return;
    
    widget_init(&tb->base, WIDGET_TEXTBOX, &textbox_widget_class);
    memset(tb->buffer, 0, WIDGET_TEXTBOX_MAX_LEN);
    tb->cursor_pos = 0;
    tb->cursor_visible = 1;
    tb->blink_ticks = 0;
    tb->password_mode = 0;
    tb->readonly = 0;
    tb->on_change = NULL;
    tb->on_submit = NULL;
    
    tb->base.width = 120;
    tb->base.height = 20;
    tb->base.bg_color = COLOR_BLACK;
    tb->base.fg_color = COLOR_WHITE;
    tb->base.border_color = COLOR_DARK_GRAY;
}

void textbox_widget_set_text(textbox_widget_t* tb, const char* text) {
    if (!tb || !text) return;
    strncpy(tb->buffer, text, WIDGET_TEXTBOX_MAX_LEN - 1);
    tb->cursor_pos = strlen(tb->buffer);
    tb->base.needs_redraw = 1;
}

const char* textbox_widget_get_text(textbox_widget_t* tb) {
    if (!tb) return "";
    return tb->buffer;
}

void textbox_widget_set_password_mode(textbox_widget_t* tb, uint8 enabled) {
    if (!tb) return;
    tb->password_mode = enabled;
    tb->base.needs_redraw = 1;
}

void textbox_widget_set_readonly(textbox_widget_t* tb, uint8 readonly) {
    if (!tb) return;
    tb->readonly = readonly;
    tb->base.needs_redraw = 1;
}

// ============================================================================
// Checkbox Widget
// ============================================================================

static void checkbox_draw(widget_t* w) {
    checkbox_t* cb = (checkbox_t*)w;
    if (!cb) return;
    
    // Draw checkbox box
    uint32 box_x = cb->base.x;
    uint32 box_y = cb->base.y;
    uint32 box_size = 14;
    
    draw_filled_rect(box_x, box_y, box_size, box_size, cb->base.bg_color);
    draw_rect(box_x, box_y, box_size, box_size, cb->base.border_color);
    
    // Draw checkmark if checked
    if (cb->checked) {
        draw_text(box_x + 2, box_y + 2, "X", COLOR_GREEN);
    }
    
    // Draw label
    if (cb->label) {
        draw_text(box_x + box_size + 4, box_y + 2, cb->label, cb->base.fg_color);
    }
}

static void checkbox_handle_event(widget_t* w, const input_event_t* event) {
    checkbox_t* cb = (checkbox_t*)w;
    if (!cb || !cb->base.enabled) return;
    
    if (event->type == INPUT_EVENT_MOUSE_PRESS) {
        if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
            if (widget_contains_point(w, event->data.mouse.x, event->data.mouse.y)) {
                cb->checked = !cb->checked;
                if (cb->on_toggle) {
                    cb->on_toggle(w, cb->checked);
                }
                cb->base.needs_redraw = 1;
            }
        }
    }
}

static const widget_class_t checkbox_class = {
    .draw = checkbox_draw,
    .handle_event = checkbox_handle_event,
    .destroy = NULL
};

void checkbox_init(checkbox_t* cb, const char* label) {
    if (!cb) return;
    
    widget_init(&cb->base, WIDGET_CHECKBOX, &checkbox_class);
    cb->label = label;
    cb->checked = 0;
    cb->on_toggle = NULL;
    
    // Auto-size
    cb->base.width = 14 + (label ? strlen(label) * 8 + 4 : 0);
    cb->base.height = 14;
    cb->base.bg_color = COLOR_WHITE;
    cb->base.fg_color = COLOR_BLACK;
    cb->base.border_color = COLOR_DARK_GRAY;
}

void checkbox_set_checked(checkbox_t* cb, uint8 checked) {
    if (!cb) return;
    cb->checked = checked;
    cb->base.needs_redraw = 1;
}

uint8 checkbox_is_checked(checkbox_t* cb) {
    return cb ? cb->checked : 0;
}

// ============================================================================
// Progress Bar Widget
// ============================================================================

static void progress_draw(widget_t* w) {
    progress_t* prog = (progress_t*)w;
    if (!prog) return;
    
    // Background
    draw_filled_rect(prog->base.x, prog->base.y, 
                    prog->base.width, prog->base.height, 
                    prog->base.bg_color);
    
    // Border
    draw_rect(prog->base.x, prog->base.y, 
             prog->base.width, prog->base.height, 
             prog->base.border_color);
    
    // Fill bar
    if (prog->max_value > 0) {
        uint32 fill_width = (prog->base.width - 4) * prog->value / prog->max_value;
        draw_filled_rect(prog->base.x + 2, prog->base.y + 2, 
                        fill_width, prog->base.height - 4, 
                        prog->fill_color);
    }
    
    // Show percentage
    if (prog->show_percent && prog->base.width >= 48) {
        uint32 percent = prog->max_value > 0 ? 
                        (prog->value * 100 / prog->max_value) : 0;
        char text[8];
        text[0] = (percent / 100) + '0';
        text[1] = ((percent / 10) % 10) + '0';
        text[2] = (percent % 10) + '0';
        text[3] = '%';
        text[4] = '\0';
        
        uint32 text_x = prog->base.x + (prog->base.width - 32) / 2;
        uint32 text_y = prog->base.y + (prog->base.height - 8) / 2;
        draw_text(text_x, text_y, text, prog->base.fg_color);
    }
}

static const widget_class_t progress_class = {
    .draw = progress_draw,
    .handle_event = NULL,
    .destroy = NULL
};

void progress_init(progress_t* prog) {
    if (!prog) return;
    
    widget_init(&prog->base, WIDGET_PROGRESS, &progress_class);
    prog->value = 0;
    prog->max_value = 100;
    prog->show_percent = 1;
    prog->fill_color = COLOR_LIGHT_CYAN;
    
    prog->base.width = 100;
    prog->base.height = 16;
    prog->base.bg_color = COLOR_DARK_GRAY;
    prog->base.border_color = COLOR_WHITE;
}

void progress_set_value(progress_t* prog, uint32 value) {
    if (!prog) return;
    prog->value = value > prog->max_value ? prog->max_value : value;
    prog->base.needs_redraw = 1;
}

void progress_set_max(progress_t* prog, uint32 max_value) {
    if (!prog) return;
    prog->max_value = max_value;
    if (prog->value > max_value) prog->value = max_value;
    prog->base.needs_redraw = 1;
}

void progress_set_show_percent(progress_t* prog, uint8 show) {
    if (!prog) return;
    prog->show_percent = show;
    prog->base.needs_redraw = 1;
}

// ============================================================================
// Slider Widget
// ============================================================================

static void slider_draw(widget_t* w) {
    slider_t* slider = (slider_t*)w;
    if (!slider) return;
    
    // Track
    uint32 track_y = slider->base.y + (slider->base.height - 4) / 2;
    draw_filled_rect(slider->base.x, track_y, slider->base.width, 4, COLOR_DARK_GRAY);
    
    // Handle position
    uint32 range = slider->max_value - slider->min_value;
    uint32 handle_x = slider->base.x;
    if (range > 0) {
        handle_x = slider->base.x + 
                  (slider->value - slider->min_value) * 
                  (slider->base.width - slider->handle_size) / range;
    }
    
    // Handle
    draw_filled_rect(handle_x, slider->base.y, 
                    slider->handle_size, slider->base.height, 
                    slider->base.bg_color);
    draw_rect(handle_x, slider->base.y, 
             slider->handle_size, slider->base.height, 
             slider->base.border_color);
}

static void slider_handle_event(widget_t* w, const input_event_t* event) {
    slider_t* slider = (slider_t*)w;
    if (!slider || !slider->base.enabled) return;
    
    if (event->type == INPUT_EVENT_MOUSE_PRESS ||
        (event->type == INPUT_EVENT_MOUSE_MOVE && 
         (event->data.mouse.buttons & MOUSE_BUTTON_LEFT))) {
        
        sint32 mx = event->data.mouse.x;
        if (mx >= slider->base.x && 
            mx < (sint32)(slider->base.x + slider->base.width)) {
            
            // Calculate new value based on mouse position
            uint32 rel_x = mx - slider->base.x;
            uint32 range = slider->max_value - slider->min_value;
            
            sint32 new_value = slider->min_value + 
                              (sint32)(rel_x * range / slider->base.width);
            
            if (new_value < slider->min_value) new_value = slider->min_value;
            if (new_value > slider->max_value) new_value = slider->max_value;
            
            if (new_value != slider->value) {
                slider->value = new_value;
                if (slider->on_change) {
                    slider->on_change(w, slider->value);
                }
                slider->base.needs_redraw = 1;
            }
        }
    }
}

static const widget_class_t slider_class = {
    .draw = slider_draw,
    .handle_event = slider_handle_event,
    .destroy = NULL
};

void slider_init(slider_t* slider, sint32 min, sint32 max) {
    if (!slider) return;
    
    widget_init(&slider->base, WIDGET_SLIDER, &slider_class);
    slider->min_value = min;
    slider->max_value = max;
    slider->value = min;
    slider->handle_size = 12;
    slider->on_change = NULL;
    
    slider->base.width = 100;
    slider->base.height = 20;
    slider->base.bg_color = COLOR_LIGHT_GRAY;
    slider->base.border_color = COLOR_DARK_GRAY;
}

void slider_set_value(slider_t* slider, sint32 value) {
    if (!slider) return;
    if (value < slider->min_value) value = slider->min_value;
    if (value > slider->max_value) value = slider->max_value;
    slider->value = value;
    slider->base.needs_redraw = 1;
}

sint32 slider_get_value(slider_t* slider) {
    return slider ? slider->value : 0;
}
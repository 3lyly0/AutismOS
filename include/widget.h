#ifndef WIDGET_H
#define WIDGET_H

#include "types.h"
#include "input_events.h"

/*
 * Widget System - AutismOS
 * 
 * A simple, object-oriented widget system for GUI components.
 * Supports: Label, Button, Panel, Textbox, Checkbox, ProgressBar
 */

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct widget_s widget_t;
typedef struct widget_class_s widget_class_t;

// ============================================================================
// Widget Types
// ============================================================================

typedef enum {
    WIDGET_NONE = 0,
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_PANEL,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX,
    WIDGET_PROGRESS,
    WIDGET_SLIDER,
    WIDGET_LIST,
    WIDGET_CUSTOM
} widget_type_t;

// ============================================================================
// Widget State
// ============================================================================

typedef enum {
    WIDGET_STATE_NORMAL = 0,
    WIDGET_STATE_HOVERED,
    WIDGET_STATE_PRESSED,
    WIDGET_STATE_FOCUSED,
    WIDGET_STATE_DISABLED
} widget_state_t;

// ============================================================================
// Alignment
// ============================================================================

typedef enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER,
    ALIGN_RIGHT
} halign_t;

typedef enum {
    ALIGN_TOP = 0,
    ALIGN_MIDDLE,
    ALIGN_BOTTOM
} valign_t;

// ============================================================================
// Widget Base Structure
// ============================================================================

typedef void (*widget_draw_fn)(widget_t* w);
typedef void (*widget_handle_event_fn)(widget_t* w, const input_event_t* event);
typedef void (*widget_destroy_fn)(widget_t* w);

struct widget_s {
    // Type and virtual table
    widget_type_t type;
    const widget_class_t* vtable;
    
    // Geometry
    sint32 x;
    sint32 y;
    uint32 width;
    uint32 height;
    
    // State
    widget_state_t state;
    uint8 visible;
    uint8 enabled;
    uint8 needs_redraw;
    
    // Style (can be overridden from theme)
    uint8 bg_color;
    uint8 fg_color;
    uint8 border_color;
    
    // Parent/Child relationship
    widget_t* parent;
    widget_t** children;
    uint32 child_count;
    uint32 child_capacity;
    
    // User data
    void* user_data;
    const char* id;
};

// ============================================================================
// Widget Virtual Table (for polymorphism)
// ============================================================================

struct widget_class_s {
    widget_draw_fn draw;
    widget_handle_event_fn handle_event;
    widget_destroy_fn destroy;
};

// ============================================================================
// Base Widget Functions
// ============================================================================

void widget_init(widget_t* w, widget_type_t type, const widget_class_t* vtable);
void widget_set_position(widget_t* w, sint32 x, sint32 y);
void widget_set_size(widget_t* w, uint32 width, uint32 height);
void widget_set_colors(widget_t* w, uint8 bg, uint8 fg, uint8 border);
void widget_set_visible(widget_t* w, uint8 visible);
void widget_set_enabled(widget_t* w, uint8 enabled);

void widget_add_child(widget_t* parent, widget_t* child);
void widget_remove_child(widget_t* parent, widget_t* child);

void widget_draw(widget_t* w);
void widget_handle_event(widget_t* w, const input_event_t* event);
void widget_destroy(widget_t* w);

uint8 widget_contains_point(widget_t* w, sint32 x, sint32 y);
widget_t* widget_find_at(widget_t* root, sint32 x, sint32 y);

// ============================================================================
// Label Widget
// ============================================================================

typedef struct {
    widget_t base;
    const char* text;
    uint8 text_color;
    halign_t halign;
    valign_t valign;
} label_t;

void label_init(label_t* label, const char* text);
void label_set_text(label_t* label, const char* text);

// ============================================================================
// Button Widget
// ============================================================================

typedef void (*button_click_fn)(widget_t* button, void* user_data);

typedef struct {
    widget_t base;
    const char* text;
    button_click_fn on_click;
    void* click_data;
    uint8 pressed;
} button_t;

void button_init(button_t* btn, const char* text);
void button_set_click_handler(button_t* btn, button_click_fn handler, void* user_data);

// ============================================================================
// Panel Widget (container)
// ============================================================================

typedef struct {
    widget_t base;
    uint8 draw_border;
    uint8 fill_background;
    const char* title;  // Optional title bar
} panel_t;

void panel_init(panel_t* panel);
void panel_set_title(panel_t* panel, const char* title);

// ============================================================================
// Textbox Widget (enhanced)
// ============================================================================

#define WIDGET_TEXTBOX_MAX_LEN 256

typedef struct {
    widget_t base;
    char buffer[WIDGET_TEXTBOX_MAX_LEN];
    uint32 cursor_pos;
    uint32 cursor_visible;
    uint32 blink_ticks;
    uint8 password_mode;  // Show * instead of text
    uint8 readonly;
    void (*on_change)(widget_t* tb, const char* text);
    void (*on_submit)(widget_t* tb, const char* text);
} textbox_widget_t;

void textbox_widget_init(textbox_widget_t* tb);
void textbox_widget_set_text(textbox_widget_t* tb, const char* text);
const char* textbox_widget_get_text(textbox_widget_t* tb);
void textbox_widget_set_password_mode(textbox_widget_t* tb, uint8 enabled);
void textbox_widget_set_readonly(textbox_widget_t* tb, uint8 readonly);

// ============================================================================
// Checkbox Widget
// ============================================================================

typedef struct {
    widget_t base;
    const char* label;
    uint8 checked;
    void (*on_toggle)(widget_t* cb, uint8 checked);
} checkbox_t;

void checkbox_init(checkbox_t* cb, const char* label);
void checkbox_set_checked(checkbox_t* cb, uint8 checked);
uint8 checkbox_is_checked(checkbox_t* cb);

// ============================================================================
// Progress Bar Widget
// ============================================================================

typedef struct {
    widget_t base;
    uint32 value;
    uint32 max_value;
    uint8 show_percent;
    uint8 fill_color;
} progress_t;

void progress_init(progress_t* prog);
void progress_set_value(progress_t* prog, uint32 value);
void progress_set_max(progress_t* prog, uint32 max_value);
void progress_set_show_percent(progress_t* prog, uint8 show);

// ============================================================================
// Slider Widget
// ============================================================================

typedef struct {
    widget_t base;
    sint32 value;
    sint32 min_value;
    sint32 max_value;
    uint8 handle_size;
    void (*on_change)(widget_t* slider, sint32 value);
} slider_t;

void slider_init(slider_t* slider, sint32 min, sint32 max);
void slider_set_value(slider_t* slider, sint32 value);
sint32 slider_get_value(slider_t* slider);

#endif // WIDGET_H
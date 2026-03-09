#ifndef DESKTOP_H
#define DESKTOP_H

#include "types.h"

#define MAX_WINDOWS 8

typedef struct {
    sint32 x;
    sint32 y;
    sint32 width;
    sint32 height;
} rect_t;

typedef struct {
    uint32 titlebar_height;
    uint32 border_thickness;
    uint32 shadow_size;
    uint32 close_button_size;
    uint32 resize_grip_size;
    uint32 content_padding;
} desktop_chrome_t;

typedef enum {
    DESKTOP_HIT_NONE = 0,
    DESKTOP_HIT_CLIENT = 1,
    DESKTOP_HIT_TITLEBAR = 2,
    DESKTOP_HIT_CLOSE = 3,
    DESKTOP_HIT_MAXIMIZE = 4,
    DESKTOP_HIT_RESIZE = 5
} desktop_hit_region_t;

#define WINDOW_FLAG_VISIBLE    0x01
#define WINDOW_FLAG_FOCUSED    0x02
#define WINDOW_FLAG_DRAGGABLE  0x04
#define WINDOW_FLAG_RESIZABLE  0x08
#define WINDOW_FLAG_MAXIMIZED  0x10

// Forward declaration for window structure
typedef struct window_s window_t;

// Window structure
struct window_s {
    uint32 id;
    uint32 x, y;
    uint32 width, height;
    char title[32];
    uint8 visible;
    uint8 focused;
    uint8 border_color;
    uint8 title_color;
    uint32 flags;
    uint32 min_width;
    uint32 min_height;
    uint32 restore_x;
    uint32 restore_y;
    uint32 restore_width;
    uint32 restore_height;
    void (*draw_content)(window_t* w);
    void (*handle_key)(window_t* w, char key);
    void (*handle_mouse)(window_t* w, sint32 local_x, sint32 local_y, uint8 buttons);
    void* app_data;
};

// Desktop state
typedef struct {
    window_t windows[MAX_WINDOWS];
    uint32 window_count;
    uint32 focused_window;
    int mouse_x, mouse_y;
    int last_mouse_x, last_mouse_y;
    uint8 mouse_buttons;
    uint8 initialized;
    uint8 needs_redraw;
    uint8 start_menu_open;
    uint32 active_window;
    desktop_hit_region_t active_hit;
    sint32 drag_offset_x;
    sint32 drag_offset_y;
    uint32 resize_origin_width;
    uint32 resize_origin_height;
} desktop_t;

// Initialize desktop
void desktop_init(void);

// Mark the desktop shell as the active interaction mode
void desktop_activate(void);

// Check whether keyboard/mouse input should target the desktop shell
uint8 is_desktop_mode(void);
void desktop_set_dirty(void);
const desktop_chrome_t* desktop_get_chrome(void);

// Get desktop state
desktop_t* desktop_get_state(void);

// Create a new window
window_t* desktop_create_window(const char* title, uint32 x, uint32 y, uint32 w, uint32 h);

// Close a window
void desktop_close_window(uint32 window_id);

// Draw the entire desktop
void desktop_draw(void);

// Handle mouse event
void desktop_handle_mouse(int x, int y, uint8 buttons);

// Handle keyboard event
void desktop_handle_key(char key);

// Get focused window
window_t* desktop_get_focused(void);

// Focus a window
void desktop_focus_window(uint32 window_id);
void desktop_move_window(window_t* w, sint32 x, sint32 y);
void desktop_resize_window(window_t* w, uint32 width, uint32 height);
void desktop_toggle_maximize(window_t* w);
void desktop_get_window_content_rect(const window_t* w, rect_t* rect);
desktop_hit_region_t desktop_hit_test_window(const window_t* w, sint32 x, sint32 y);

// Draw window frame
void desktop_draw_window(window_t* w);

// Update mouse position
void desktop_update_mouse(int dx, int dy, uint8 buttons);

// Draw mouse cursor directly (used by mouse IRQ handlers)
void desktop_draw_mouse_direct(void);

#endif

#ifndef DESKTOP_H
#define DESKTOP_H

#include "types.h"

#define MAX_WINDOWS 8
#define WINDOW_TITLE_HEIGHT 1

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
    void (*draw_content)(window_t* w);
    void (*handle_key)(window_t* w, char key);
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
} desktop_t;

// Initialize desktop
void desktop_init(void);

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

// Draw window frame
void desktop_draw_window(window_t* w);

// Update mouse position
void desktop_update_mouse(int dx, int dy, uint8 buttons);

// Get desktop state
desktop_t* desktop_get_state(void);

#endif

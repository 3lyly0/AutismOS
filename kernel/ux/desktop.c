#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "video.h"
#include "notepad.h"
#include "calculator.h"
#include "sysinfo.h"

static desktop_t g_desktop;
static uint32 next_window_id = 1;

// Character to draw for mouse cursor (0x1A = right arrow pointer)
#define MOUSE_CHAR 0x1A

void desktop_init(void) {
    memset(&g_desktop, 0, sizeof(desktop_t));
    g_desktop.mouse_x = 40;
    g_desktop.mouse_y = 12;
    g_desktop.last_mouse_x = 40;
    g_desktop.last_mouse_y = 12;
    g_desktop.window_count = 0;
    g_desktop.focused_window = 0;
    g_desktop.initialized = 1;
    g_desktop.needs_redraw = 1;  // Initial draw needed
}

void desktop_set_dirty(void) {
    g_desktop.needs_redraw = 1;
}

desktop_t* desktop_get_state(void) {
    return &g_desktop;
}

window_t* desktop_create_window(const char* title, uint32 x, uint32 y, uint32 w, uint32 h) {
    if (g_desktop.window_count >= MAX_WINDOWS) {
        return NULL;
    }
    
    window_t* win = &g_desktop.windows[g_desktop.window_count];
    memset(win, 0, sizeof(window_t));
    
    win->id = next_window_id++;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = 1;
    win->focused = 0;
    win->border_color = COLOR_LIGHT_GRAY;
    win->title_color = COLOR_WHITE;
    
    if (title) {
        strncpy(win->title, title, sizeof(win->title) - 1);
    }
    
    g_desktop.window_count++;
    
    // Focus the new window
    desktop_focus_window(win->id);
    
    return win;
}

void desktop_close_window(uint32 window_id) {
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == window_id) {
            // Shift remaining windows
            for (uint32 j = i; j < g_desktop.window_count - 1; j++) {
                g_desktop.windows[j] = g_desktop.windows[j + 1];
            }
            g_desktop.window_count--;
            
            // Focus another window if available
            if (g_desktop.window_count > 0) {
                desktop_focus_window(g_desktop.windows[0].id);
            } else {
                g_desktop.focused_window = 0;
            }
            break;
        }
    }
}

void desktop_focus_window(uint32 window_id) {
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == window_id) {
            g_desktop.windows[i].focused = 1;
            g_desktop.windows[i].border_color = COLOR_LIGHT_CYAN;
            g_desktop.focused_window = window_id;
        } else {
            g_desktop.windows[i].focused = 0;
            g_desktop.windows[i].border_color = COLOR_DARK_GRAY;
        }
    }
}

window_t* desktop_get_focused(void) {
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == g_desktop.focused_window) {
            return &g_desktop.windows[i];
        }
    }
    return NULL;
}

void desktop_draw_window(window_t* w) {
    if (!w || !w->visible) return;
    
    // Clear window area
    graphics_clear_region(w->x, w->y, w->width, w->height, COLOR_BLACK);
    
    // Draw border
    draw_rect(w->x, w->y, w->width, w->height, w->border_color);
    
    // Draw title bar
    if (w->title[0]) {
        // Title bar background
        volatile char* video = (volatile char*)0xB8000;
        uint8 title_bg = w->focused ? COLOR_BLUE : COLOR_DARK_GRAY;
        
        for (uint32 col = w->x + 1; col < w->x + w->width - 1; col++) {
            int index = (w->y * 80 + col) * 2;
            video[index + 1] = (title_bg << 4) | COLOR_WHITE;
        }
        
        // Draw title text
        uint32 title_x = w->x + 2;
        for (uint32 i = 0; w->title[i] && title_x < w->x + w->width - 4; i++, title_x++) {
            int index = (w->y * 80 + title_x) * 2;
            video[index] = w->title[i];
            video[index + 1] = (title_bg << 4) | COLOR_WHITE;
        }
        
        // Draw close button [X]
        uint32 close_x = w->x + w->width - 3;
        int close_idx = (w->y * 80 + close_x) * 2;
        video[close_idx] = 'X';
        video[close_idx + 1] = (COLOR_RED << 4) | COLOR_WHITE;
    }
    
    // Call app-specific draw function
    if (w->draw_content) {
        w->draw_content(w);
    }
}

// Saved character behind mouse cursor
static char saved_char = ' ';
static uint8 saved_attr = (COLOR_DARK_GRAY << 4) | COLOR_WHITE;

// Draw only the mouse cursor (fast update) - called from IRQ
void desktop_draw_mouse_direct(void) {
    if (!g_desktop.initialized) return;
    
    volatile char* video = (volatile char*)0xB8000;
    
    int mx = g_desktop.mouse_x;
    int my = g_desktop.mouse_y;
    int lx = g_desktop.last_mouse_x;
    int ly = g_desktop.last_mouse_y;
    
    // Only update if mouse moved
    if (mx == lx && my == ly) return;
    
    // Restore old position
    if (lx >= 0 && lx < 80 && ly >= 0 && ly < 25) {
        int index = (ly * 80 + lx) * 2;
        video[index] = saved_char;
        video[index + 1] = saved_attr;
    }
    
    // Save new position content
    if (mx >= 0 && mx < 80 && my >= 0 && my < 25) {
        int index = (my * 80 + mx) * 2;
        saved_char = video[index];
        saved_attr = video[index + 1];
        
        // Draw cursor (white block for visibility)
        video[index] = 0xDB;  // Full block
        video[index + 1] = (COLOR_BLACK << 4) | COLOR_WHITE;
    }
    
    g_desktop.last_mouse_x = mx;
    g_desktop.last_mouse_y = my;
}

// For internal use
static void desktop_draw_mouse(void) {
    desktop_draw_mouse_direct();
}

void desktop_draw(void) {
    if (!g_desktop.needs_redraw) {
        // Only update mouse cursor
        desktop_draw_mouse();
        return;
    }
    
    g_desktop.needs_redraw = 0;
    
    // Fill background (only non-window areas will show this)
    volatile char* video = (volatile char*)0xB8000;
    
    // Draw desktop background (rows 0-23, avoiding clearing windows)
    for (int row = 0; row < 24; row++) {
        for (int col = 0; col < 80; col++) {
            int index = (row * 80 + col) * 2;
            video[index] = ' ';
            video[index + 1] = (COLOR_DARK_GRAY << 4) | COLOR_DARK_GRAY;
        }
    }
    
    // Draw taskbar at bottom
    for (int col = 0; col < 80; col++) {
        int index = (24 * 80 + col) * 2;
        video[index] = ' ';
        video[index + 1] = (COLOR_BLUE << 4) | COLOR_WHITE;
    }
    
    // Draw "Start" on taskbar
    const char* start = "[AutismOS]";
    for (int i = 0; start[i]; i++) {
        int index = (24 * 80 + i + 1) * 2;
        video[index] = start[i];
        video[index + 1] = (COLOR_BLUE << 4) | COLOR_LIGHT_GREEN;
    }
    
    // Draw Start Menu if open
    if (g_desktop.start_menu_open) {
        // Menu background (above taskbar)
        for (int row = 18; row < 24; row++) {
            for (int col = 0; col < 20; col++) {
                int index = (row * 80 + col) * 2;
                video[index] = ' ';
                video[index + 1] = (COLOR_LIGHT_GRAY << 4) | COLOR_BLACK;
            }
        }
        
        // Menu border
        for (int col = 0; col < 20; col++) {
            int index = (18 * 80 + col) * 2;
            video[index] = '-';
            video[index + 1] = (COLOR_LIGHT_GRAY << 4) | COLOR_DARK_GRAY;
        }
        
        // Menu items
        const char* items[] = {"1. Notepad", "2. Calculator", "3. System Info", "4. Close Menu", NULL};
        for (int i = 0; items[i]; i++) {
            int row = 19 + i;
            for (int j = 0; items[i][j]; j++) {
                int index = (row * 80 + 1 + j) * 2;
                video[index] = items[i][j];
                video[index + 1] = (COLOR_LIGHT_GRAY << 4) | COLOR_BLACK;
            }
        }
    }
    
    // Draw all windows
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        desktop_draw_window(&g_desktop.windows[i]);
    }
    
    // Draw mouse cursor and save background
    int mx = g_desktop.mouse_x;
    int my = g_desktop.mouse_y;
    if (mx >= 0 && mx < 80 && my >= 0 && my < 25) {
        int index = (my * 80 + mx) * 2;
        // Save what's behind the cursor (after drawing windows)
        saved_char = video[index];
        saved_attr = video[index + 1];
        // Draw cursor (white block)
        video[index] = 0xDB;
        video[index + 1] = (COLOR_BLACK << 4) | COLOR_WHITE;
    }
    
    g_desktop.last_mouse_x = mx;
    g_desktop.last_mouse_y = my;
}

void desktop_update_mouse(int dx, int dy, uint8 buttons) {
    g_desktop.mouse_x += dx;
    g_desktop.mouse_y -= dy;  // Invert Y
    
    if (g_desktop.mouse_x < 0) g_desktop.mouse_x = 0;
    if (g_desktop.mouse_x >= 80) g_desktop.mouse_x = 79;
    if (g_desktop.mouse_y < 0) g_desktop.mouse_y = 0;
    if (g_desktop.mouse_y >= 25) g_desktop.mouse_y = 24;
    
    g_desktop.mouse_buttons = buttons;
}

void desktop_handle_mouse(int x, int y, uint8 buttons) {
    static uint8 last_buttons = 0;
    
    // Only handle on button press (not hold)
    if ((buttons & 0x01) && !(last_buttons & 0x01)) {
        
        // Check if clicking on Start Menu items
        if (g_desktop.start_menu_open) {
            if (x >= 0 && x < 20 && y >= 19 && y < 23) {
                int item = y - 19;
                if (item == 0) {
                    // Notepad
                    notepad_create();
                } else if (item == 1) {
                    // Calculator
                    calculator_create();
                } else if (item == 2) {
                    // System Info
                    sysinfo_create();
                }
                // Close menu after selection
                g_desktop.start_menu_open = 0;
                g_desktop.needs_redraw = 1;
                last_buttons = buttons;
                return;
            }
            // Click anywhere else closes menu
            g_desktop.start_menu_open = 0;
            g_desktop.needs_redraw = 1;
        }
        
        // Check if clicking on taskbar Start button
        if (y == 24 && x >= 1 && x <= 11) {
            g_desktop.start_menu_open = !g_desktop.start_menu_open;
            g_desktop.needs_redraw = 1;
            last_buttons = buttons;
            return;
        }
        
        // Check windows in reverse order (top window first)
        for (int i = g_desktop.window_count - 1; i >= 0; i--) {
            window_t* w = &g_desktop.windows[i];
            if (!w->visible) continue;
            
            if (x >= (int)w->x && x < (int)(w->x + w->width) &&
                y >= (int)w->y && y < (int)(w->y + w->height)) {
                
                // Check if close button clicked
                if (y == (int)w->y && x == (int)(w->x + w->width - 3)) {
                    desktop_close_window(w->id);
                    g_desktop.needs_redraw = 1;
                    last_buttons = buttons;
                    return;
                }
                
                // Focus this window
                desktop_focus_window(w->id);
                g_desktop.needs_redraw = 1;
                last_buttons = buttons;
                return;
            }
        }
    }
    last_buttons = buttons;
}

void desktop_handle_key(char key) {
    // Handle Start menu keyboard shortcuts
    if (g_desktop.start_menu_open) {
        if (key == '1') {
            notepad_create();
            g_desktop.start_menu_open = 0;
            g_desktop.needs_redraw = 1;
            return;
        } else if (key == '2') {
            calculator_create();
            g_desktop.start_menu_open = 0;
            g_desktop.needs_redraw = 1;
            return;
        } else if (key == '3') {
            sysinfo_create();
            g_desktop.start_menu_open = 0;
            g_desktop.needs_redraw = 1;
            return;
        } else if (key == '4' || key == 27) {  // 4 or Escape
            g_desktop.start_menu_open = 0;
            g_desktop.needs_redraw = 1;
            return;
        }
    }
    
    // Global hotkeys (when menu is closed)
    // Use Windows key, ESC or backtick (`) for Start menu
    if (key == (char)0x80 || key == 27 || key == '`') {
        // Toggle start menu
        g_desktop.start_menu_open = !g_desktop.start_menu_open;
        g_desktop.needs_redraw = 1;
        return;
    }
    
    // Ctrl+Q = Reboot (halt CPU - QEMU will restart or close)
    if (key == 17) { // Ctrl+Q
        asm volatile("cli");
        // Triple fault to reboot
        uint8* null_ptr = (uint8*)0;
        *null_ptr = 0;
    }
    
    // Escape closes menu or quits focused window
    // Already handled above for menu toggle
    
    // Pass to focused window
    window_t* focused = desktop_get_focused();
    if (focused && focused->handle_key) {
        focused->handle_key(focused, key);
        // App handle_key already redraws its window - no full redraw needed
    }
}

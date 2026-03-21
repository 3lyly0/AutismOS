#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "video.h"
#include "notepad.h"
#include "calculator.h"
#include "sysinfo.h"
#include "input_manager.h"
#include "theme.h"
#include "widget.h"

/*
 * Desktop Shell - AutismOS (Refactored)
 * 
 * Uses the new input_manager for mouse/keyboard events,
 * theme system for colors, and widget system for UI components.
 */

// ============================================================================
// Constants
// ============================================================================

#define TASKBAR_HEIGHT 20
#define START_BUTTON_W 58
#define START_BUTTON_H 14
#define START_MENU_W 154
#define START_MENU_H 84
#define WINDOW_MIN_WIDTH 96
#define WINDOW_MIN_HEIGHT 72
#define TASKBAR_BUTTON_W 68
#define TASKBAR_BUTTON_SPACING 6

// ============================================================================
// Desktop State
// ============================================================================

static const desktop_chrome_t g_chrome = {
    18, /* titlebar_height */
    1,  /* border_thickness */
    4,  /* shadow_size */
    12, /* close_button_size */
    12, /* resize_grip_size */
    8   /* content_padding */
};

static desktop_t g_desktop;
static uint32 next_window_id = 1;
static volatile uint8 g_desktop_mode = 0;

// ============================================================================
// Helper Functions
// ============================================================================

static int point_in_rect(sint32 x, sint32 y, sint32 rx, sint32 ry, sint32 rw, sint32 rh) {
    return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

static window_t* desktop_find_window_by_id(uint32 window_id) {
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == window_id) {
            return &g_desktop.windows[i];
        }
    }
    return NULL;
}

static void desktop_get_workspace_rect(rect_t* rect) {
    if (!rect) return;
    rect->x = 0;
    rect->y = 0;
    rect->width = SCREEN_WIDTH;
    rect->height = SCREEN_HEIGHT - TASKBAR_HEIGHT;
}

// ============================================================================
// Input Event Handler (registered with input_manager)
// ============================================================================

static void desktop_input_handler(const input_event_t* event, void* user_data) {
    (void)user_data;
    
    if (!g_desktop_mode) return;
    
    switch (event->type) {
        case INPUT_EVENT_MOUSE_MOVE: {
            // Get position directly from input_manager (already processed)
            sint32 new_x = event->data.mouse.x;
            sint32 new_y = event->data.mouse.y;
            
            // Only redraw if mouse actually moved significantly
            if (new_x != g_desktop.mouse_x || new_y != g_desktop.mouse_y) {
                g_desktop.mouse_x = new_x;
                g_desktop.mouse_y = new_y;
                g_desktop.mouse_buttons = event->data.mouse.buttons;
                
                // Continue window interaction if active
                if (g_desktop.active_window != 0 && (event->data.mouse.buttons & MOUSE_BUTTON_LEFT)) {
                    window_t* w = desktop_find_window_by_id(g_desktop.active_window);
                    if (w) {
                        if (g_desktop.active_hit == DESKTOP_HIT_TITLEBAR && (w->flags & WINDOW_FLAG_DRAGGABLE)) {
                            desktop_move_window(w, new_x - g_desktop.drag_offset_x, new_y - g_desktop.drag_offset_y);
                        } else if (g_desktop.active_hit == DESKTOP_HIT_RESIZE && (w->flags & WINDOW_FLAG_RESIZABLE)) {
                            uint32 new_width = (uint32)(new_x - (sint32)w->x + 4);
                            uint32 new_height = (uint32)(new_y - (sint32)w->y + 4);
                            desktop_resize_window(w, new_width, new_height);
                        }
                    }
                }
                
                desktop_set_dirty();
            }
            break;
        }
            
        case INPUT_EVENT_MOUSE_PRESS: {
            g_desktop.mouse_x = event->data.mouse.x;
            g_desktop.mouse_y = event->data.mouse.y;
            g_desktop.mouse_buttons = event->data.mouse.buttons;
            
            if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
                // Check start button
                if (point_in_rect(g_desktop.mouse_x, g_desktop.mouse_y, 8, SCREEN_HEIGHT - TASKBAR_HEIGHT + 3, START_BUTTON_W, START_BUTTON_H)) {
                    g_desktop.start_menu_open = !g_desktop.start_menu_open;
                    desktop_set_dirty();
                    return;
                }
                
                // Check menu clicks
                if (g_desktop.start_menu_open) {
                    sint32 menu_y = SCREEN_HEIGHT - TASKBAR_HEIGHT - START_MENU_H;
                    if (point_in_rect(g_desktop.mouse_x, g_desktop.mouse_y, 18, menu_y + 24, 134, 18)) {
                        notepad_create();
                        g_desktop.start_menu_open = 0;
                    } else if (point_in_rect(g_desktop.mouse_x, g_desktop.mouse_y, 18, menu_y + 45, 134, 18)) {
                        calculator_create();
                        g_desktop.start_menu_open = 0;
                    } else if (point_in_rect(g_desktop.mouse_x, g_desktop.mouse_y, 18, menu_y + 66, 134, 18)) {
                        sysinfo_create();
                        g_desktop.start_menu_open = 0;
                    }
                    desktop_set_dirty();
                    return;
                }
                
                // Check taskbar window buttons
                sint32 button_x = 74;
                for (uint32 i = 0; i < g_desktop.window_count; i++) {
                    if (point_in_rect(g_desktop.mouse_x, g_desktop.mouse_y, button_x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15)) {
                        desktop_focus_window(g_desktop.windows[i].id);
                        desktop_set_dirty();
                        return;
                    }
                    button_x += TASKBAR_BUTTON_W + TASKBAR_BUTTON_SPACING;
                }
                
                // Check window interactions
                for (sint32 i = (sint32)g_desktop.window_count - 1; i >= 0; i--) {
                    window_t* w = &g_desktop.windows[i];
                    desktop_hit_region_t hit = desktop_hit_test_window(w, g_desktop.mouse_x, g_desktop.mouse_y);
                    if (hit == DESKTOP_HIT_NONE) continue;
                    
                    if (hit == DESKTOP_HIT_CLOSE) {
                        desktop_close_window(w->id);
                        desktop_set_dirty();
                        return;
                    }
                    
                    desktop_focus_window(w->id);
                    w = desktop_get_focused();
                    
                    if (hit == DESKTOP_HIT_MAXIMIZE) {
                        desktop_toggle_maximize(w);
                        desktop_set_dirty();
                        return;
                    }
                    
                    // Begin window interaction
                    g_desktop.active_window = w->id;
                    g_desktop.active_hit = hit;
                    g_desktop.drag_offset_x = g_desktop.mouse_x - (sint32)w->x;
                    g_desktop.drag_offset_y = g_desktop.mouse_y - (sint32)w->y;
                    
                    if (hit == DESKTOP_HIT_CLIENT && w && w->handle_mouse) {
                        rect_t content;
                        desktop_get_window_content_rect(w, &content);
                        w->handle_mouse(w, g_desktop.mouse_x - content.x, g_desktop.mouse_y - content.y, g_desktop.mouse_buttons);
                    }
                    
                    desktop_set_dirty();
                    return;
                }
            }
            break;
        }
            
        case INPUT_EVENT_MOUSE_RELEASE: {
            g_desktop.mouse_buttons = event->data.mouse.buttons;
            
            if (event->data.mouse.button_changed == MOUSE_BUTTON_LEFT) {
                g_desktop.active_window = 0;
                g_desktop.active_hit = DESKTOP_HIT_NONE;
            }
            break;
        }
            
        case INPUT_EVENT_KEY_PRESS: {
            // Handle keyboard shortcuts
            if (event->data.key.keycode == KEY_ESCAPE) {
                if (g_desktop.start_menu_open) {
                    g_desktop.start_menu_open = 0;
                    desktop_set_dirty();
                    return;
                }
            } else if (event->data.key.keycode == KEY_F1) {
                notepad_create();
                desktop_set_dirty();
                return;
            } else if (event->data.key.keycode == KEY_F2) {
                calculator_create();
                desktop_set_dirty();
                return;
            } else if (event->data.key.keycode == KEY_F3) {
                sysinfo_create();
                desktop_set_dirty();
                return;
            } else if (event->data.key.keycode == KEY_TAB && event->data.key.ctrl) {
                desktop_cycle_focus();
                desktop_set_dirty();
                return;
            }
            
            // Forward character to keyboard handler
            if (event->data.key.character) {
                desktop_handle_key(event->data.key.character);
            }
            break;
        }
            
        default:
            break;
    }
}

// ============================================================================
// App Launchers
// ============================================================================

static void desktop_launch_notepad(void) { notepad_create(); }
static void desktop_launch_calculator(void) { calculator_create(); }
static void desktop_launch_sysinfo(void) { sysinfo_create(); }

static void desktop_cycle_focus(void) {
    if (g_desktop.window_count == 0) return;
    
    if (g_desktop.focused_window == 0) {
        desktop_focus_window(g_desktop.windows[g_desktop.window_count - 1].id);
        return;
    }
    
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == g_desktop.focused_window) {
            if (i == 0) {
                desktop_focus_window(g_desktop.windows[g_desktop.window_count - 1].id);
            } else {
                desktop_focus_window(g_desktop.windows[i - 1].id);
            }
            return;
        }
    }
}

static void desktop_close_focused(void) {
    if (g_desktop.focused_window != 0) {
        desktop_close_window(g_desktop.focused_window);
    }
}

static void desktop_toggle_launcher(void) {
    g_desktop.start_menu_open = !g_desktop.start_menu_open;
    desktop_set_dirty();
}

static void desktop_bring_to_front(uint32 index) {
    if (index >= g_desktop.window_count || index == g_desktop.window_count - 1) return;
    
    window_t moved = g_desktop.windows[index];
    for (uint32 i = index; i + 1 < g_desktop.window_count; i++) {
        g_desktop.windows[i] = g_desktop.windows[i + 1];
    }
    g_desktop.windows[g_desktop.window_count - 1] = moved;
}

// ============================================================================
// Drawing Functions (using theme system)
// ============================================================================

static void desktop_draw_wallpaper(void) {
    uint8 bg_primary = theme_color(THEME_BG_PRIMARY);
    uint8 bg_secondary = theme_color(THEME_BG_SECONDARY);
    uint8 accent = theme_color(THEME_ACCENT_PRIMARY);
    
    // Gradient background
    for (sint32 y = 0; y < SCREEN_HEIGHT - TASKBAR_HEIGHT; y++) {
        uint8 color = bg_primary;
        if (y > 46 && y < 108) {
            color = bg_secondary;
        } else if (y >= 108) {
            color = accent;
        }
        draw_line(0, y, SCREEN_WIDTH - 1, y, color);
    }
    
    // Pattern overlay
    for (sint32 y = 10; y < SCREEN_HEIGHT - TASKBAR_HEIGHT; y += 18) {
        for (sint32 x = 8; x < SCREEN_WIDTH; x += 22) {
            draw_pixel(x, y, theme_color(THEME_FG_PRIMARY));
            draw_pixel(x + 1, y + 1, theme_color(THEME_ACCENT_SECONDARY));
        }
    }
    
    // Logo area
    uint8 logo_bg = theme_color(THEME_ACCENT_SECONDARY);
    uint8 logo_fg = theme_color(THEME_FG_PRIMARY);
    
    draw_filled_rect(192, 18, 100, 54, logo_bg);
    draw_rect(192, 18, 100, 54, theme_color(THEME_BORDER_NORMAL));
    draw_text(204, 30, "AutismOS", logo_fg);
    draw_text(204, 44, "Pixel desktop", logo_fg);
}

static void desktop_draw_taskbar(void) {
    uint8 taskbar_bg = theme_color(THEME_BG_SECONDARY);
    uint8 taskbar_border = theme_color(THEME_BORDER_NORMAL);
    uint8 text_color = theme_color(THEME_FG_PRIMARY);
    
    // Taskbar background
    draw_filled_rect(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH, TASKBAR_HEIGHT, taskbar_bg);
    draw_line(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH - 1, SCREEN_HEIGHT - TASKBAR_HEIGHT, taskbar_border);
    
    // Start button
    uint8 start_bg = g_desktop.start_menu_open ? 
                     theme_color(THEME_ACCENT_PRIMARY) : 
                     theme_color(THEME_STATE_NORMAL);
    uint8 start_fg = theme_color(THEME_FG_PRIMARY);
    
    draw_filled_rect(8, SCREEN_HEIGHT - TASKBAR_HEIGHT + 3, START_BUTTON_W, START_BUTTON_H, start_bg);
    draw_rect(8, SCREEN_HEIGHT - TASKBAR_HEIGHT + 3, START_BUTTON_W, START_BUTTON_H, taskbar_border);
    draw_text(18, SCREEN_HEIGHT - TASKBAR_HEIGHT + 7, "AutismOS", start_fg);
    
    // Window buttons in taskbar
    sint32 x = 74;
    for (uint32 i = 0; i < g_desktop.window_count && x + TASKBAR_BUTTON_W < SCREEN_WIDTH - 8; i++) {
        window_t* w = &g_desktop.windows[i];
        
        uint8 btn_bg = w->focused ? 
                       theme_color(THEME_ACCENT_SECONDARY) : 
                       theme_color(THEME_STATE_NORMAL);
        uint8 btn_fg = w->focused ? 
                       theme_color(THEME_BG_PRIMARY) : 
                       theme_color(THEME_FG_PRIMARY);
        
        draw_filled_rect((uint32)x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15, btn_bg);
        draw_rect((uint32)x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15, taskbar_border);
        draw_text((uint32)x + 4, SCREEN_HEIGHT - TASKBAR_HEIGHT + 6, w->title, btn_fg);
        x += TASKBAR_BUTTON_W + TASKBAR_BUTTON_SPACING;
    }
    
    // Status text
    draw_text(248, SCREEN_HEIGHT - 13, "GUI shell", text_color);
}

static void desktop_draw_menu(void) {
    if (!g_desktop.start_menu_open) return;
    
    uint8 menu_bg = theme_color(THEME_BG_TERTIARY);
    uint8 menu_border = theme_color(THEME_BORDER_NORMAL);
    uint8 header_bg = theme_color(THEME_ACCENT_PRIMARY);
    uint8 text_color = theme_color(THEME_FG_PRIMARY);
    
    sint32 menu_x = 8;
    sint32 menu_y = SCREEN_HEIGHT - TASKBAR_HEIGHT - START_MENU_H;
    
    // Menu background
    draw_filled_rect(menu_x, menu_y, START_MENU_W, START_MENU_H, menu_bg);
    draw_rect(menu_x, menu_y, START_MENU_W, START_MENU_H, menu_border);
    
    // Header
    draw_filled_rect(menu_x, menu_y, START_MENU_W, 18, header_bg);
    draw_text(menu_x + 8, menu_y + 6, "Launch apps", text_color);
    
    // Menu items
    sint32 item_y = menu_y + 24;
    
    // Notepad
    draw_filled_rect(menu_x + 10, item_y, 134, 18, theme_color(THEME_ACCENT_PRIMARY));
    draw_rect(menu_x + 10, item_y, 134, 18, menu_border);
    draw_text(menu_x + 16, item_y + 4, "Notepad", text_color);
    draw_text(menu_x + 82, item_y + 4, "press 1", theme_color(THEME_FG_SECONDARY));
    item_y += 21;
    
    // Calculator
    draw_filled_rect(menu_x + 10, item_y, 134, 18, COLOR_GREEN);
    draw_rect(menu_x + 10, item_y, 134, 18, menu_border);
    draw_text(menu_x + 16, item_y + 4, "Calculator", text_color);
    draw_text(menu_x + 82, item_y + 4, "press 2", theme_color(THEME_FG_SECONDARY));
    item_y += 21;
    
    // System Info
    draw_filled_rect(menu_x + 10, item_y, 134, 18, COLOR_CYAN);
    draw_rect(menu_x + 10, item_y, 134, 18, menu_border);
    draw_text(menu_x + 16, item_y + 4, "System Info", text_color);
    draw_text(menu_x + 82, item_y + 4, "press 3", theme_color(THEME_FG_SECONDARY));
}

static void desktop_draw_resize_grip(const window_t* w) {
    sint32 grip_x = (sint32)(w->x + w->width - g_chrome.resize_grip_size - 3);
    sint32 grip_y = (sint32)(w->y + w->height - g_chrome.resize_grip_size - 3);
    uint8 grip_color = theme_color(THEME_BORDER_NORMAL);
    
    for (sint32 i = 0; i < 4; i++) {
        draw_line(grip_x + i * 3, grip_y + g_chrome.resize_grip_size - 1, 
                  grip_x + g_chrome.resize_grip_size - 1, grip_y + i * 3, grip_color);
    }
}

static void desktop_draw_maximize_button(const window_t* w) {
    uint32 x = w->x + w->width - g_chrome.close_button_size - 22;
    uint8 fill = (w->flags & WINDOW_FLAG_MAXIMIZED) ? 
                 theme_color(THEME_STATE_SUCCESS) : 
                 theme_color(THEME_STATE_NORMAL);
    
    draw_filled_rect(x, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, fill);
    draw_rect(x, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, 
              theme_color(THEME_BORDER_NORMAL));
    draw_rect(x + 3, w->y + 6, g_chrome.close_button_size - 6, g_chrome.close_button_size - 6, 
              theme_color(THEME_FG_PRIMARY));
}

void desktop_draw_window(window_t* w) {
    rect_t content;
    if (!w || !w->visible) return;
    
    desktop_get_window_content_rect(w, &content);
    
    // Get theme colors for window
    uint8 titlebar_color, border_color, content_color;
    theme_get_window_colors(w->focused, &titlebar_color, &border_color, &content_color);
    
    // Shadow
    draw_filled_rect(w->x + g_chrome.shadow_size, w->y + g_chrome.shadow_size, 
                    w->width, w->height, theme_color(THEME_BG_SECONDARY));
    
    // Window background
    draw_filled_rect(w->x, w->y, w->width, w->height, content_color);
    draw_rect(w->x, w->y, w->width, w->height, border_color);
    
    // Titlebar
    draw_filled_rect(w->x, w->y, w->width, g_chrome.titlebar_height, titlebar_color);
    draw_line((sint32)w->x, (sint32)(w->y + g_chrome.titlebar_height), 
              (sint32)(w->x + w->width - 1), (sint32)(w->y + g_chrome.titlebar_height), 
              theme_color(THEME_BORDER_NORMAL));
    
    // Title text
    draw_text(w->x + 6, w->y + 5, w->title, w->title_color);
    
    // Maximize button
    desktop_draw_maximize_button(w);
    
    // Close button
    draw_filled_rect(w->x + w->width - g_chrome.close_button_size - 6, w->y + 3, 
                    g_chrome.close_button_size, g_chrome.close_button_size, 
                    theme_color(THEME_STATE_ERROR));
    draw_rect(w->x + w->width - g_chrome.close_button_size - 6, w->y + 3, 
             g_chrome.close_button_size, g_chrome.close_button_size, 
             theme_color(THEME_BORDER_NORMAL));
    draw_text(w->x + w->width - g_chrome.close_button_size - 3, w->y + 6, "X", 
              theme_color(THEME_FG_PRIMARY));
    
    // Content area
    draw_filled_rect((uint32)content.x - 1, (uint32)content.y - 1, 
                    (uint32)content.width + 2, (uint32)content.height + 2, 
                    theme_color(THEME_BG_PRIMARY));
    
    // Resize grip
    if (w->flags & WINDOW_FLAG_RESIZABLE) {
        desktop_draw_resize_grip(w);
    }
    
    // Custom content drawing
    if (w->draw_content) {
        w->draw_content(w);
    }
}

// ============================================================================
// Desktop API
// ============================================================================

void desktop_init(void) {
    memset(&g_desktop, 0, sizeof(desktop_t));
    
    // Initialize mouse from input_manager
    g_desktop.mouse_x = input_get_mouse_x();
    g_desktop.mouse_y = input_get_mouse_y();
    g_desktop.last_mouse_x = g_desktop.mouse_x;
    g_desktop.last_mouse_y = g_desktop.mouse_y;
    g_desktop.initialized = 1;
    g_desktop.needs_redraw = 1;
    
    // Register input listener with high priority
    input_add_listener("desktop", desktop_input_handler, NULL, 10);
}

void desktop_activate(void) {
    g_desktop_mode = 1;
}

uint8 is_desktop_mode(void) {
    return g_desktop_mode;
}

void desktop_set_dirty(void) {
    g_desktop.needs_redraw = 1;
}

const desktop_chrome_t* desktop_get_chrome(void) {
    return &g_chrome;
}

desktop_t* desktop_get_state(void) {
    return &g_desktop;
}

// ============================================================================
// Window Management
// ============================================================================

void desktop_move_window(window_t* w, sint32 x, sint32 y) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);
    
    if (!w || (w->flags & WINDOW_FLAG_MAXIMIZED)) return;
    
    if (x < workspace.x) x = workspace.x;
    if (y < workspace.y) y = workspace.y;
    if (x + (sint32)w->width > workspace.width) x = workspace.width - (sint32)w->width;
    if (y + (sint32)g_chrome.titlebar_height > workspace.height) y = workspace.height - (sint32)g_chrome.titlebar_height;
    
    w->x = (uint32)x;
    w->y = (uint32)y;
}

void desktop_resize_window(window_t* w, uint32 width, uint32 height) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);
    
    if (!w || (w->flags & WINDOW_FLAG_MAXIMIZED)) return;
    
    if (width < w->min_width) width = w->min_width;
    if (height < w->min_height) height = w->min_height;
    
    if ((sint32)(w->x + width) > workspace.width) width = (uint32)(workspace.width - (sint32)w->x);
    if ((sint32)(w->y + height) > workspace.height) height = (uint32)(workspace.height - (sint32)w->y);
    
    if (width < w->min_width) width = w->min_width;
    if (height < w->min_height) height = w->min_height;
    
    w->width = width;
    w->height = height;
}

void desktop_toggle_maximize(window_t* w) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);
    
    if (!w) return;
    
    if (w->flags & WINDOW_FLAG_MAXIMIZED) {
        w->flags &= ~WINDOW_FLAG_MAXIMIZED;
        w->x = w->restore_x;
        w->y = w->restore_y;
        w->width = w->restore_width;
        w->height = w->restore_height;
    } else {
        w->restore_x = w->x;
        w->restore_y = w->y;
        w->restore_width = w->width;
        w->restore_height = w->height;
        w->flags |= WINDOW_FLAG_MAXIMIZED;
        w->x = 6;
        w->y = 6;
        w->width = (uint32)(workspace.width - 12);
        w->height = (uint32)(workspace.height - 12);
    }
}

void desktop_get_window_content_rect(const window_t* w, rect_t* rect) {
    if (!w || !rect) return;
    
    rect->x = (sint32)w->x + (sint32)g_chrome.content_padding;
    rect->y = (sint32)w->y + (sint32)g_chrome.titlebar_height + (sint32)g_chrome.content_padding;
    rect->width = (sint32)w->width - (sint32)(g_chrome.content_padding * 2);
    rect->height = (sint32)w->height - (sint32)g_chrome.titlebar_height - (sint32)(g_chrome.content_padding * 2);
}

desktop_hit_region_t desktop_hit_test_window(const window_t* w, sint32 x, sint32 y) {
    if (!w || !w->visible || !point_in_rect(x, y, (sint32)w->x, (sint32)w->y, (sint32)w->width, (sint32)w->height)) {
        return DESKTOP_HIT_NONE;
    }
    
    // Close button
    if (point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.close_button_size - 6), (sint32)(w->y + 3),
        (sint32)g_chrome.close_button_size, (sint32)g_chrome.close_button_size)) {
        return DESKTOP_HIT_CLOSE;
    }
    
    // Maximize button
    if (point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.close_button_size - 22), (sint32)(w->y + 3),
        (sint32)g_chrome.close_button_size, (sint32)g_chrome.close_button_size)) {
        return DESKTOP_HIT_MAXIMIZE;
    }
    
    // Resize grip
    if ((w->flags & WINDOW_FLAG_RESIZABLE) &&
        point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.resize_grip_size - 4), 
                     (sint32)(w->y + w->height - g_chrome.resize_grip_size - 4),
                     (sint32)(g_chrome.resize_grip_size + 4), (sint32)(g_chrome.resize_grip_size + 4))) {
        return DESKTOP_HIT_RESIZE;
    }
    
    // Titlebar
    if (point_in_rect(x, y, (sint32)w->x, (sint32)w->y, (sint32)w->width, (sint32)g_chrome.titlebar_height)) {
        return DESKTOP_HIT_TITLEBAR;
    }
    
    return DESKTOP_HIT_CLIENT;
}

window_t* desktop_create_window(const char* title, uint32 x, uint32 y, uint32 w, uint32 h) {
    if (g_desktop.window_count >= MAX_WINDOWS) return NULL;
    
    window_t* win = &g_desktop.windows[g_desktop.window_count++];
    memset(win, 0, sizeof(window_t));
    
    win->id = next_window_id++;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->visible = 1;
    win->focused = 0;
    win->flags = WINDOW_FLAG_VISIBLE | WINDOW_FLAG_DRAGGABLE | WINDOW_FLAG_RESIZABLE;
    win->min_width = WINDOW_MIN_WIDTH;
    win->min_height = WINDOW_MIN_HEIGHT;
    win->border_color = theme_color(THEME_ACCENT_PRIMARY);
    win->title_color = theme_color(THEME_FG_PRIMARY);
    
    if (title) {
        strncpy(win->title, title, sizeof(win->title) - 1);
    }
    
    desktop_resize_window(win, w, h);
    desktop_focus_window(win->id);
    desktop_set_dirty();
    return win;
}

void desktop_close_window(uint32 window_id) {
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == window_id) {
            for (uint32 j = i; j + 1 < g_desktop.window_count; j++) {
                g_desktop.windows[j] = g_desktop.windows[j + 1];
            }
            g_desktop.window_count--;
            break;
        }
    }
    
    if (g_desktop.active_window == window_id) {
        g_desktop.active_window = 0;
        g_desktop.active_hit = DESKTOP_HIT_NONE;
    }
    
    g_desktop.focused_window = 0;
    if (g_desktop.window_count > 0) {
        desktop_focus_window(g_desktop.windows[g_desktop.window_count - 1].id);
    }
    desktop_set_dirty();
}

void desktop_focus_window(uint32 window_id) {
    uint32 found_index = MAX_WINDOWS;
    
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == window_id) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == MAX_WINDOWS) return;
    
    desktop_bring_to_front(found_index);
    
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        window_t* win = &g_desktop.windows[i];
        win->focused = (i == g_desktop.window_count - 1);
        win->border_color = win->focused ? 
                           theme_color(THEME_BORDER_FOCUSED) : 
                           theme_color(THEME_BORDER_NORMAL);
        if (win->focused) {
            g_desktop.focused_window = win->id;
        }
    }
    
    desktop_set_dirty();
}

window_t* desktop_get_focused(void) {
    return desktop_find_window_by_id(g_desktop.focused_window);
}

// ============================================================================
// Drawing
// ============================================================================

void desktop_draw_mouse_direct(void) {
    desktop_set_dirty();
}

void desktop_draw(void) {
    if (!g_desktop.initialized || !g_desktop.needs_redraw) return;
    
    g_desktop.needs_redraw = 0;
    
    // Update mouse position from input_manager
    g_desktop.mouse_x = input_get_mouse_x();
    g_desktop.mouse_y = input_get_mouse_y();
    
    // Draw all layers
    desktop_draw_wallpaper();
    desktop_draw_taskbar();
    desktop_draw_menu();
    
    // Draw windows
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        desktop_draw_window(&g_desktop.windows[i]);
    }
    
    // Draw cursor
    draw_cursor((uint32)g_desktop.mouse_x, (uint32)g_desktop.mouse_y, 1);
    graphics_present();
    
    g_desktop.last_mouse_x = g_desktop.mouse_x;
    g_desktop.last_mouse_y = g_desktop.mouse_y;
}

// ============================================================================
// Legacy Functions (kept for compatibility)
// ============================================================================

void desktop_update_mouse(int dx, int dy, uint8 buttons) {
    (void)dx;
    (void)dy;
    (void)buttons;
}

void desktop_handle_mouse(int x, int y, uint8 buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

void desktop_handle_key(char key) {
    if (g_desktop.start_menu_open) {
        if (key == '1') {
            notepad_create();
            g_desktop.start_menu_open = 0;
            desktop_set_dirty();
            return;
        }
        if (key == '2') {
            calculator_create();
            g_desktop.start_menu_open = 0;
            desktop_set_dirty();
            return;
        }
        if (key == '3') {
            sysinfo_create();
            g_desktop.start_menu_open = 0;
            desktop_set_dirty();
            return;
        }
        if (key == 27) {
            g_desktop.start_menu_open = 0;
            desktop_set_dirty();
            return;
        }
    }
    
    if (key == (char)0x80 || key == 27 || key == '`' || key == 20) {
        desktop_toggle_launcher();
        return;
    }
    
    if (key == (char)0x83 || key == '1') {
        desktop_launch_notepad();
        desktop_set_dirty();
        return;
    }
    
    if (key == (char)0x84 || key == '2') {
        desktop_launch_calculator();
        desktop_set_dirty();
        return;
    }
    
    if (key == (char)0x85 || key == '3') {
        desktop_launch_sysinfo();
        desktop_set_dirty();
        return;
    }
    
    if (key == (char)0x86 || key == 14) {
        desktop_launch_notepad();
        desktop_launch_calculator();
        desktop_launch_sysinfo();
        desktop_set_dirty();
        return;
    }
    
    if (key == (char)0x88 || key == '\t') {
        desktop_cycle_focus();
        desktop_set_dirty();
        return;
    }
    
    if (key == 23 || key == (char)0x81) {
        desktop_close_focused();
        return;
    }
    
    if (key == 13 || key == (char)0x87) {
        window_t* focused_for_max = desktop_get_focused();
        if (focused_for_max) {
            desktop_toggle_maximize(focused_for_max);
            desktop_set_dirty();
        }
        return;
    }
    
    if (key == 18 || key == (char)0x82) {
        window_t* focused_for_raise = desktop_get_focused();
        if (focused_for_raise) {
            desktop_focus_window(focused_for_raise->id);
            desktop_set_dirty();
        }
        return;
    }
    
    if (key == 17) {
        asm volatile("cli");
        for (;;) {
            asm volatile("hlt");
        }
    }
    
    // Forward to focused window
    window_t* focused = desktop_get_focused();
    if (focused && focused->handle_key) {
        focused->handle_key(focused, key);
        desktop_set_dirty();
    }
}
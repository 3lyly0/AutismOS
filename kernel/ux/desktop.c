#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "video.h"
#include "notepad.h"
#include "calculator.h"
#include "sysinfo.h"

#define DESKTOP_BG_TOP COLOR_BLUE
#define DESKTOP_BG_MID COLOR_LIGHT_BLUE
#define DESKTOP_BG_BOTTOM COLOR_CYAN
#define TASKBAR_HEIGHT 20
#define START_X 8
#define START_Y (SCREEN_HEIGHT - TASKBAR_HEIGHT + 3)
#define START_W 58
#define START_H 14
#define START_MENU_X 8
#define START_MENU_Y (SCREEN_HEIGHT - TASKBAR_HEIGHT - 88)
#define START_MENU_W 154
#define START_MENU_H 84
#define WINDOW_MIN_WIDTH 96
#define WINDOW_MIN_HEIGHT 72
#define TASKBAR_BUTTON_X 74
#define TASKBAR_BUTTON_W 68
#define TASKBAR_BUTTON_SPACING 6

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
    if (!rect) {
        return;
    }

    rect->x = 0;
    rect->y = 0;
    rect->width = SCREEN_WIDTH;
    rect->height = SCREEN_HEIGHT - TASKBAR_HEIGHT;
}

static void desktop_bring_to_front(uint32 index) {
    if (index >= g_desktop.window_count || index == g_desktop.window_count - 1) {
        return;
    }

    window_t moved = g_desktop.windows[index];
    for (uint32 i = index; i + 1 < g_desktop.window_count; i++) {
        g_desktop.windows[i] = g_desktop.windows[i + 1];
    }
    g_desktop.windows[g_desktop.window_count - 1] = moved;
}

static void desktop_draw_wallpaper(void) {
    for (sint32 y = 0; y < SCREEN_HEIGHT - TASKBAR_HEIGHT; y++) {
        uint8 color = DESKTOP_BG_TOP;
        if (y > 46 && y < 108) {
            color = DESKTOP_BG_MID;
        } else if (y >= 108) {
            color = DESKTOP_BG_BOTTOM;
        }
        draw_line(0, y, SCREEN_WIDTH - 1, y, color);
    }

    for (sint32 y = 10; y < SCREEN_HEIGHT - TASKBAR_HEIGHT; y += 18) {
        for (sint32 x = 8; x < SCREEN_WIDTH; x += 22) {
            draw_pixel(x, y, COLOR_WHITE);
            draw_pixel(x + 1, y + 1, COLOR_LIGHT_CYAN);
        }
    }

    draw_filled_rect(192, 18, 100, 54, COLOR_LIGHT_CYAN);
    draw_rect(192, 18, 100, 54, COLOR_WHITE);
    draw_text(204, 30, "AutismOS", COLOR_BLUE);
    draw_text(204, 44, "Pixel desktop", COLOR_BLUE);
}

static void desktop_draw_taskbar_windows(void) {
    sint32 x = TASKBAR_BUTTON_X;

    for (uint32 i = 0; i < g_desktop.window_count && x + TASKBAR_BUTTON_W < SCREEN_WIDTH - 8; i++) {
        window_t* w = &g_desktop.windows[i];
        uint8 fill = w->focused ? COLOR_LIGHT_CYAN : COLOR_LIGHT_GRAY;
        uint8 text = w->focused ? COLOR_BLUE : COLOR_BLACK;

        draw_filled_rect((uint32)x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15, fill);
        draw_rect((uint32)x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15, COLOR_WHITE);
        draw_text((uint32)x + 4, SCREEN_HEIGHT - TASKBAR_HEIGHT + 6, w->title, text);
        x += TASKBAR_BUTTON_W + TASKBAR_BUTTON_SPACING;
    }
}

static void desktop_draw_background(void) {
    desktop_draw_wallpaper();

    draw_filled_rect(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH, TASKBAR_HEIGHT, COLOR_DARK_GRAY);
    draw_line(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH - 1, SCREEN_HEIGHT - TASKBAR_HEIGHT, COLOR_WHITE);

    draw_filled_rect(START_X, START_Y, START_W, START_H, g_desktop.start_menu_open ? COLOR_LIGHT_CYAN : COLOR_LIGHT_GRAY);
    draw_rect(START_X, START_Y, START_W, START_H, COLOR_WHITE);
    draw_text(START_X + 10, START_Y + 4, "AutismOS", COLOR_BLACK);
    desktop_draw_taskbar_windows();
    draw_text(248, SCREEN_HEIGHT - 13, "GUI shell", COLOR_WHITE);
}

static void desktop_draw_menu_tile(sint32 x, sint32 y, uint8 color, const char* title, const char* hint) {
    draw_filled_rect((uint32)x, (uint32)y, 134, 18, color);
    draw_rect((uint32)x, (uint32)y, 134, 18, COLOR_WHITE);
    draw_text((uint32)x + 6, (uint32)y + 4, title, COLOR_WHITE);
    draw_text((uint32)x + 72, (uint32)y + 4, hint, COLOR_LIGHT_GRAY);
}

static void desktop_draw_menu(void) {
    if (!g_desktop.start_menu_open) {
        return;
    }

    draw_filled_rect(START_MENU_X, START_MENU_Y, START_MENU_W, START_MENU_H, COLOR_LIGHT_GRAY);
    draw_rect(START_MENU_X, START_MENU_Y, START_MENU_W, START_MENU_H, COLOR_WHITE);
    draw_filled_rect(START_MENU_X, START_MENU_Y, START_MENU_W, 18, COLOR_BLUE);
    draw_text(START_MENU_X + 8, START_MENU_Y + 6, "Launch apps", COLOR_WHITE);

    desktop_draw_menu_tile(START_MENU_X + 10, START_MENU_Y + 24, COLOR_BLUE, "Notepad", "press 1");
    desktop_draw_menu_tile(START_MENU_X + 10, START_MENU_Y + 45, COLOR_GREEN, "Calculator", "press 2");
    desktop_draw_menu_tile(START_MENU_X + 10, START_MENU_Y + 66, COLOR_CYAN, "System Info", "press 3");
}

static void desktop_draw_resize_grip(const window_t* w) {
    sint32 grip_x = (sint32)(w->x + w->width - g_chrome.resize_grip_size - 3);
    sint32 grip_y = (sint32)(w->y + w->height - g_chrome.resize_grip_size - 3);

    for (sint32 i = 0; i < 4; i++) {
        draw_line(grip_x + i * 3, grip_y + g_chrome.resize_grip_size - 1, grip_x + g_chrome.resize_grip_size - 1, grip_y + i * 3, COLOR_DARK_GRAY);
    }
}

static void desktop_draw_maximize_button(const window_t* w) {
    uint32 x = w->x + w->width - g_chrome.close_button_size - 22;
    uint8 fill = (w->flags & WINDOW_FLAG_MAXIMIZED) ? COLOR_LIGHT_GREEN : COLOR_LIGHT_GRAY;

    draw_filled_rect(x, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, fill);
    draw_rect(x, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, COLOR_WHITE);
    draw_rect(x + 3, w->y + 6, g_chrome.close_button_size - 6, g_chrome.close_button_size - 6, COLOR_BLACK);
}

void desktop_init(void) {
    memset(&g_desktop, 0, sizeof(desktop_t));
    g_desktop.mouse_x = SCREEN_WIDTH / 2;
    g_desktop.mouse_y = SCREEN_HEIGHT / 2;
    g_desktop.last_mouse_x = g_desktop.mouse_x;
    g_desktop.last_mouse_y = g_desktop.mouse_y;
    g_desktop.initialized = 1;
    g_desktop.needs_redraw = 1;
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

void desktop_move_window(window_t* w, sint32 x, sint32 y) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);

    if (!w || (w->flags & WINDOW_FLAG_MAXIMIZED)) {
        return;
    }

    if (x < workspace.x) {
        x = workspace.x;
    }
    if (y < workspace.y) {
        y = workspace.y;
    }
    if (x + (sint32)w->width > workspace.width) {
        x = workspace.width - (sint32)w->width;
    }
    if (y + (sint32)g_chrome.titlebar_height > workspace.height) {
        y = workspace.height - (sint32)g_chrome.titlebar_height;
    }

    w->x = (uint32)x;
    w->y = (uint32)y;
}

void desktop_resize_window(window_t* w, uint32 width, uint32 height) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);

    if (!w || (w->flags & WINDOW_FLAG_MAXIMIZED)) {
        return;
    }

    if (width < w->min_width) {
        width = w->min_width;
    }
    if (height < w->min_height) {
        height = w->min_height;
    }

    if ((sint32)(w->x + width) > workspace.width) {
        width = (uint32)(workspace.width - (sint32)w->x);
    }
    if ((sint32)(w->y + height) > workspace.height) {
        height = (uint32)(workspace.height - (sint32)w->y);
    }

    if (width < w->min_width) {
        width = w->min_width;
    }
    if (height < w->min_height) {
        height = w->min_height;
    }

    w->width = width;
    w->height = height;
}

void desktop_toggle_maximize(window_t* w) {
    rect_t workspace;
    desktop_get_workspace_rect(&workspace);

    if (!w) {
        return;
    }

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
    if (!w || !rect) {
        return;
    }

    rect->x = (sint32)w->x + (sint32)g_chrome.content_padding;
    rect->y = (sint32)w->y + (sint32)g_chrome.titlebar_height + (sint32)g_chrome.content_padding;
    rect->width = (sint32)w->width - (sint32)(g_chrome.content_padding * 2);
    rect->height = (sint32)w->height - (sint32)g_chrome.titlebar_height - (sint32)(g_chrome.content_padding * 2);
}

desktop_hit_region_t desktop_hit_test_window(const window_t* w, sint32 x, sint32 y) {
    if (!w || !w->visible || !point_in_rect(x, y, (sint32)w->x, (sint32)w->y, (sint32)w->width, (sint32)w->height)) {
        return DESKTOP_HIT_NONE;
    }

    if (point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.close_button_size - 6), (sint32)(w->y + 3),
        (sint32)g_chrome.close_button_size, (sint32)g_chrome.close_button_size)) {
        return DESKTOP_HIT_CLOSE;
    }

    if (point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.close_button_size - 22), (sint32)(w->y + 3),
        (sint32)g_chrome.close_button_size, (sint32)g_chrome.close_button_size)) {
        return DESKTOP_HIT_MAXIMIZE;
    }

    if ((w->flags & WINDOW_FLAG_RESIZABLE) &&
        point_in_rect(x, y, (sint32)(w->x + w->width - g_chrome.resize_grip_size - 4), (sint32)(w->y + w->height - g_chrome.resize_grip_size - 4),
            (sint32)(g_chrome.resize_grip_size + 4), (sint32)(g_chrome.resize_grip_size + 4))) {
        return DESKTOP_HIT_RESIZE;
    }

    if (point_in_rect(x, y, (sint32)w->x, (sint32)w->y, (sint32)w->width, (sint32)g_chrome.titlebar_height)) {
        return DESKTOP_HIT_TITLEBAR;
    }

    return DESKTOP_HIT_CLIENT;
}

window_t* desktop_create_window(const char* title, uint32 x, uint32 y, uint32 w, uint32 h) {
    if (g_desktop.window_count >= MAX_WINDOWS) {
        return NULL;
    }

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
    win->border_color = COLOR_BLUE;
    win->title_color = COLOR_WHITE;

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

    if (found_index == MAX_WINDOWS) {
        return;
    }

    desktop_bring_to_front(found_index);

    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        window_t* win = &g_desktop.windows[i];
        win->focused = (i == g_desktop.window_count - 1);
        win->border_color = win->focused ? COLOR_LIGHT_CYAN : COLOR_DARK_GRAY;
        if (win->focused) {
            g_desktop.focused_window = win->id;
        }
    }

    desktop_set_dirty();
}

window_t* desktop_get_focused(void) {
    return desktop_find_window_by_id(g_desktop.focused_window);
}

void desktop_draw_window(window_t* w) {
    rect_t content;

    if (!w || !w->visible) {
        return;
    }

    desktop_get_window_content_rect(w, &content);

    draw_filled_rect(w->x + g_chrome.shadow_size, w->y + g_chrome.shadow_size, w->width, w->height, COLOR_DARK_GRAY);
    draw_filled_rect(w->x, w->y, w->width, w->height, COLOR_LIGHT_GRAY);
    draw_rect(w->x, w->y, w->width, w->height, w->border_color);
    draw_filled_rect(w->x, w->y, w->width, g_chrome.titlebar_height, w->focused ? COLOR_BLUE : COLOR_DARK_GRAY);
    draw_line((sint32)w->x, (sint32)(w->y + g_chrome.titlebar_height), (sint32)(w->x + w->width - 1), (sint32)(w->y + g_chrome.titlebar_height), COLOR_WHITE);

    draw_text(w->x + 6, w->y + 5, w->title, w->title_color);
    desktop_draw_maximize_button(w);

    draw_filled_rect(w->x + w->width - g_chrome.close_button_size - 6, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, COLOR_RED);
    draw_rect(w->x + w->width - g_chrome.close_button_size - 6, w->y + 3, g_chrome.close_button_size, g_chrome.close_button_size, COLOR_WHITE);
    draw_text(w->x + w->width - g_chrome.close_button_size - 3, w->y + 6, "X", COLOR_WHITE);

    draw_filled_rect((uint32)content.x - 1, (uint32)content.y - 1, (uint32)content.width + 2, (uint32)content.height + 2, COLOR_WHITE);

    if (w->flags & WINDOW_FLAG_RESIZABLE) {
        desktop_draw_resize_grip(w);
    }

    if (w->draw_content) {
        w->draw_content(w);
    }
}

void desktop_draw_mouse_direct(void) {
    desktop_set_dirty();
}

void desktop_draw(void) {
    if (!g_desktop.initialized || !g_desktop.needs_redraw) {
        return;
    }

    g_desktop.needs_redraw = 0;

    desktop_draw_background();
    desktop_draw_menu();

    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        desktop_draw_window(&g_desktop.windows[i]);
    }

    draw_cursor((uint32)g_desktop.mouse_x, (uint32)g_desktop.mouse_y, 1);
    graphics_present();

    g_desktop.last_mouse_x = g_desktop.mouse_x;
    g_desktop.last_mouse_y = g_desktop.mouse_y;
}

void desktop_update_mouse(int dx, int dy, uint8 buttons) {
    sint32 new_x = g_desktop.mouse_x + dx;
    sint32 new_y = g_desktop.mouse_y - dy;

    if (new_x < 0) {
        new_x = 0;
    }
    if (new_x > SCREEN_WIDTH - 10) {
        new_x = SCREEN_WIDTH - 10;
    }
    if (new_y < 0) {
        new_y = 0;
    }
    if (new_y > SCREEN_HEIGHT - 16) {
        new_y = SCREEN_HEIGHT - 16;
    }

    if (new_x != g_desktop.mouse_x || new_y != g_desktop.mouse_y || buttons != g_desktop.mouse_buttons) {
        g_desktop.mouse_x = new_x;
        g_desktop.mouse_y = new_y;
        g_desktop.mouse_buttons = buttons;
        desktop_set_dirty();
    }
}

static void desktop_handle_menu_click(sint32 x, sint32 y) {
    if (!g_desktop.start_menu_open) {
        return;
    }

    if (point_in_rect(x, y, START_MENU_X + 10, START_MENU_Y + 24, 134, 18)) {
        notepad_create();
    } else if (point_in_rect(x, y, START_MENU_X + 10, START_MENU_Y + 45, 134, 18)) {
        calculator_create();
    } else if (point_in_rect(x, y, START_MENU_X + 10, START_MENU_Y + 66, 134, 18)) {
        sysinfo_create();
    }

    g_desktop.start_menu_open = 0;
    desktop_set_dirty();
}

static int desktop_handle_taskbar_click(sint32 x, sint32 y) {
    sint32 button_x = TASKBAR_BUTTON_X;

    if (!point_in_rect(x, y, 0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH, TASKBAR_HEIGHT)) {
        return 0;
    }

    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (point_in_rect(x, y, button_x, SCREEN_HEIGHT - TASKBAR_HEIGHT + 2, TASKBAR_BUTTON_W, 15)) {
            desktop_focus_window(g_desktop.windows[i].id);
            return 1;
        }
        button_x += TASKBAR_BUTTON_W + TASKBAR_BUTTON_SPACING;
    }

    return 0;
}

static void desktop_begin_window_interaction(window_t* w, desktop_hit_region_t hit, sint32 x, sint32 y) {
    if (!w) {
        return;
    }

    g_desktop.active_window = w->id;
    g_desktop.active_hit = hit;
    g_desktop.drag_offset_x = x - (sint32)w->x;
    g_desktop.drag_offset_y = y - (sint32)w->y;
    g_desktop.resize_origin_width = w->width;
    g_desktop.resize_origin_height = w->height;
}

static void desktop_continue_window_interaction(sint32 x, sint32 y, uint8 buttons) {
    window_t* w = desktop_find_window_by_id(g_desktop.active_window);
    if (!w || !(buttons & 0x01)) {
        return;
    }

    if (g_desktop.active_hit == DESKTOP_HIT_TITLEBAR && (w->flags & WINDOW_FLAG_DRAGGABLE)) {
        desktop_move_window(w, x - g_desktop.drag_offset_x, y - g_desktop.drag_offset_y);
        desktop_set_dirty();
    } else if (g_desktop.active_hit == DESKTOP_HIT_RESIZE && (w->flags & WINDOW_FLAG_RESIZABLE)) {
        uint32 new_width = (uint32)(x - (sint32)w->x + 4);
        uint32 new_height = (uint32)(y - (sint32)w->y + 4);
        desktop_resize_window(w, new_width, new_height);
        desktop_set_dirty();
    } else if (g_desktop.active_hit == DESKTOP_HIT_CLIENT && w->handle_mouse) {
        rect_t content;
        desktop_get_window_content_rect(w, &content);
        w->handle_mouse(w, x - content.x, y - content.y, buttons);
    }
}

void desktop_handle_mouse(int x, int y, uint8 buttons) {
    static uint8 last_buttons = 0;

    if ((buttons & 0x01) && g_desktop.active_window != 0) {
        desktop_continue_window_interaction((sint32)x, (sint32)y, buttons);
    }

    if ((buttons & 0x01) && !(last_buttons & 0x01)) {
        if (g_desktop.start_menu_open) {
            desktop_handle_menu_click((sint32)x, (sint32)y);
            last_buttons = buttons;
            return;
        }

        if (point_in_rect((sint32)x, (sint32)y, START_X, START_Y, START_W, START_H)) {
            g_desktop.start_menu_open = !g_desktop.start_menu_open;
            desktop_set_dirty();
            last_buttons = buttons;
            return;
        }

        if (desktop_handle_taskbar_click((sint32)x, (sint32)y)) {
            last_buttons = buttons;
            return;
        }

        for (sint32 i = (sint32)g_desktop.window_count - 1; i >= 0; i--) {
            window_t* w = &g_desktop.windows[i];
            desktop_hit_region_t hit = desktop_hit_test_window(w, (sint32)x, (sint32)y);
            if (hit == DESKTOP_HIT_NONE) {
                continue;
            }

            if (hit == DESKTOP_HIT_CLOSE) {
                desktop_close_window(w->id);
                last_buttons = buttons;
                return;
            }

            desktop_focus_window(w->id);
            w = desktop_get_focused();

            if (hit == DESKTOP_HIT_MAXIMIZE) {
                desktop_toggle_maximize(w);
                desktop_set_dirty();
                last_buttons = buttons;
                return;
            }

            desktop_begin_window_interaction(w, hit, (sint32)x, (sint32)y);

            if (hit == DESKTOP_HIT_CLIENT && w && w->handle_mouse) {
                rect_t content;
                desktop_get_window_content_rect(w, &content);
                w->handle_mouse(w, (sint32)x - content.x, (sint32)y - content.y, buttons);
            }

            last_buttons = buttons;
            return;
        }
    }

    if (!(buttons & 0x01) && (last_buttons & 0x01)) {
        g_desktop.active_window = 0;
        g_desktop.active_hit = DESKTOP_HIT_NONE;
    }

    last_buttons = buttons;
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

    if (key == (char)0x80 || key == 27 || key == '`') {
        g_desktop.start_menu_open = !g_desktop.start_menu_open;
        desktop_set_dirty();
        return;
    }

    if (key == 17) {
        asm volatile("cli");
        for (;;) {
            asm volatile("hlt");
        }
    }

    {
        window_t* focused = desktop_get_focused();
        if (focused && focused->handle_key) {
            focused->handle_key(focused, key);
            desktop_set_dirty();
        }
    }
}

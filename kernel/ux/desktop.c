#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "video.h"
#include "notepad.h"
#include "calculator.h"
#include "sysinfo.h"

#define DESKTOP_BG_TOP COLOR_BLUE
#define DESKTOP_BG_BOTTOM COLOR_CYAN
#define TASKBAR_HEIGHT 20
#define TITLE_BAR_HEIGHT 18
#define START_X 8
#define START_Y (SCREEN_HEIGHT - TASKBAR_HEIGHT + 3)
#define START_W 58
#define START_H 14
#define START_MENU_X 8
#define START_MENU_Y (SCREEN_HEIGHT - TASKBAR_HEIGHT - 76)
#define START_MENU_W 132
#define START_MENU_H 72
#define CLOSE_BOX_SIZE 12

static desktop_t g_desktop;
static uint32 next_window_id = 1;
static volatile uint8 g_desktop_mode = 0;

static void desktop_draw_background(void) {
    for (int y = 0; y < SCREEN_HEIGHT - TASKBAR_HEIGHT; y++) {
        uint8 color = (y < (SCREEN_HEIGHT - TASKBAR_HEIGHT) / 2) ? DESKTOP_BG_TOP : DESKTOP_BG_BOTTOM;
        draw_line(0, y, SCREEN_WIDTH - 1, y, color);
    }

    draw_filled_rect(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH, TASKBAR_HEIGHT, COLOR_DARK_GRAY);
    draw_line(0, SCREEN_HEIGHT - TASKBAR_HEIGHT, SCREEN_WIDTH - 1, SCREEN_HEIGHT - TASKBAR_HEIGHT, COLOR_WHITE);
    draw_filled_rect(START_X, START_Y, START_W, START_H, g_desktop.start_menu_open ? COLOR_LIGHT_CYAN : COLOR_LIGHT_GRAY);
    draw_rect(START_X, START_Y, START_W, START_H, COLOR_WHITE);
    draw_text(START_X + 10, START_Y + 4, "AutismOS", COLOR_BLACK);
    draw_text(220, SCREEN_HEIGHT - 13, "GUI desktop", COLOR_WHITE);
}

static void desktop_draw_menu(void) {
    if (!g_desktop.start_menu_open) {
        return;
    }

    draw_filled_rect(START_MENU_X, START_MENU_Y, START_MENU_W, START_MENU_H, COLOR_LIGHT_GRAY);
    draw_rect(START_MENU_X, START_MENU_Y, START_MENU_W, START_MENU_H, COLOR_WHITE);
    draw_filled_rect(START_MENU_X, START_MENU_Y, START_MENU_W, 16, COLOR_BLUE);
    draw_text(START_MENU_X + 6, START_MENU_Y + 5, "Launch", COLOR_WHITE);

    draw_text(START_MENU_X + 10, START_MENU_Y + 24, "1  Notepad", COLOR_BLACK);
    draw_text(START_MENU_X + 10, START_MENU_Y + 38, "2  Calculator", COLOR_BLACK);
    draw_text(START_MENU_X + 10, START_MENU_Y + 52, "3  System info", COLOR_BLACK);
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

desktop_t* desktop_get_state(void) {
    return &g_desktop;
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
    win->border_color = COLOR_BLUE;
    win->title_color = COLOR_WHITE;

    if (title) {
        strncpy(win->title, title, sizeof(win->title) - 1);
    }

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
            if (g_desktop.window_count > 0) {
                g_desktop.window_count--;
            }
            break;
        }
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
    for (uint32 i = 0; i < g_desktop.window_count; i++) {
        if (g_desktop.windows[i].id == g_desktop.focused_window) {
            return &g_desktop.windows[i];
        }
    }
    return NULL;
}

void desktop_draw_window(window_t* w) {
    if (!w || !w->visible) {
        return;
    }

    draw_filled_rect(w->x + 4, w->y + 4, w->width, w->height, COLOR_DARK_GRAY);
    draw_filled_rect(w->x, w->y, w->width, w->height, COLOR_LIGHT_GRAY);
    draw_rect(w->x, w->y, w->width, w->height, w->border_color);
    draw_filled_rect(w->x, w->y, w->width, TITLE_BAR_HEIGHT, w->focused ? COLOR_BLUE : COLOR_DARK_GRAY);

    draw_text(w->x + 6, w->y + 5, w->title, COLOR_WHITE);

    draw_filled_rect(w->x + w->width - 18, w->y + 3, CLOSE_BOX_SIZE, CLOSE_BOX_SIZE, COLOR_RED);
    draw_rect(w->x + w->width - 18, w->y + 3, CLOSE_BOX_SIZE, CLOSE_BOX_SIZE, COLOR_WHITE);
    draw_text(w->x + w->width - 15, w->y + 6, "X", COLOR_WHITE);

    draw_filled_rect(w->x + 1, w->y + TITLE_BAR_HEIGHT, w->width - 2, w->height - TITLE_BAR_HEIGHT - 1, COLOR_WHITE);

    if (w->draw_content) {
        w->draw_content(w);
    }
}

void desktop_draw_mouse_direct(void) {
    desktop_set_dirty();
}

void desktop_draw(void) {
    if (!g_desktop.initialized) {
        return;
    }

    if (!g_desktop.needs_redraw) {
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
    int new_x = g_desktop.mouse_x + dx;
    int new_y = g_desktop.mouse_y - dy;

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

static int point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

void desktop_handle_mouse(int x, int y, uint8 buttons) {
    static uint8 last_buttons = 0;

    if ((buttons & 0x01) && !(last_buttons & 0x01)) {
        if (g_desktop.start_menu_open) {
            if (point_in_rect(x, y, START_MENU_X, START_MENU_Y + 18, START_MENU_W, 18)) {
                notepad_create();
                g_desktop.start_menu_open = 0;
                desktop_set_dirty();
                last_buttons = buttons;
                return;
            }
            if (point_in_rect(x, y, START_MENU_X, START_MENU_Y + 32, START_MENU_W, 18)) {
                calculator_create();
                g_desktop.start_menu_open = 0;
                desktop_set_dirty();
                last_buttons = buttons;
                return;
            }
            if (point_in_rect(x, y, START_MENU_X, START_MENU_Y + 46, START_MENU_W, 18)) {
                sysinfo_create();
                g_desktop.start_menu_open = 0;
                desktop_set_dirty();
                last_buttons = buttons;
                return;
            }
            g_desktop.start_menu_open = 0;
            desktop_set_dirty();
        }

        if (point_in_rect(x, y, START_X, START_Y, START_W, START_H)) {
            g_desktop.start_menu_open = !g_desktop.start_menu_open;
            desktop_set_dirty();
            last_buttons = buttons;
            return;
        }

        for (int i = (int)g_desktop.window_count - 1; i >= 0; i--) {
            window_t* w = &g_desktop.windows[i];
            if (!w->visible) {
                continue;
            }

            if (point_in_rect(x, y, (int)w->x, (int)w->y, (int)w->width, (int)w->height)) {
                if (point_in_rect(x, y, (int)(w->x + w->width - 18), (int)(w->y + 3), CLOSE_BOX_SIZE, CLOSE_BOX_SIZE)) {
                    desktop_close_window(w->id);
                    last_buttons = buttons;
                    return;
                }

                desktop_focus_window(w->id);
                last_buttons = buttons;
                return;
            }
        }
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

    window_t* focused = desktop_get_focused();
    if (focused && focused->handle_key) {
        focused->handle_key(focused, key);
        desktop_set_dirty();
    }
}

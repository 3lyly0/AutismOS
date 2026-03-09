#include "sysinfo.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

extern volatile uint64 g_timer_ticks;

static sysinfo_state_t g_sysinfo_states[MAX_WINDOWS];
static uint32 g_sysinfo_count = 0;

static void uint_to_str(uint32 num, char* buf) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    char tmp[16];
    int i = 0;
    while (num > 0 && i < 15) {
        tmp[i++] = '0' + (num % 10);
        num /= 10;
    }

    int pos = 0;
    while (i > 0) {
        buf[pos++] = tmp[--i];
    }
    buf[pos] = '\0';
}

static sysinfo_state_t* get_state(window_t* win) {
    if (!win) {
        return NULL;
    }

    if (win->app_data) {
        return (sysinfo_state_t*)win->app_data;
    }

    if (g_sysinfo_count >= MAX_WINDOWS) {
        return NULL;
    }

    sysinfo_state_t* state = &g_sysinfo_states[g_sysinfo_count++];
    memset(state, 0, sizeof(sysinfo_state_t));
    win->app_data = state;
    return state;
}

void sysinfo_draw(window_t* win) {
    sysinfo_state_t* state = get_state(win);
    rect_t content;
    if (!win || !state) {
        return;
    }

    char num[16];
    desktop_get_window_content_rect(win, &content);

    draw_text((uint32)content.x + 4, (uint32)content.y + 8, "AutismOS build", COLOR_BLACK);
    draw_text((uint32)content.x + 4, (uint32)content.y + 24, "Display: VGA 320x200", COLOR_BLACK);
    draw_text((uint32)content.x + 4, (uint32)content.y + 40, "Desktop: Pixel GUI", COLOR_BLACK);
    draw_text((uint32)content.x + 4, (uint32)content.y + 54, "Windows: drag, resize, maximize", COLOR_BLACK);

    draw_text((uint32)content.x + 4, (uint32)content.y + 74, "Ticks:", COLOR_BLACK);
    uint_to_str((uint32)g_timer_ticks, num);
    draw_text((uint32)content.x + 48, (uint32)content.y + 74, num, COLOR_BLUE);

    draw_text((uint32)content.x + 4, (uint32)content.y + 90, "Status:", COLOR_BLACK);
    draw_text((uint32)content.x + 48, (uint32)content.y + 90, "Running", COLOR_GREEN);
    draw_progress_bar((uint32)content.x + 4, (uint32)content.y + 108, 100, (uint32)(g_timer_ticks % 100), COLOR_GREEN);

    draw_text((uint32)content.x + 4, (uint32)content.y + 124, "R refreshes", COLOR_DARK_GRAY);
    state->update_counter++;
}

void sysinfo_handle_key(window_t* win, char key) {
    if ((key == 'r' || key == 'R') && win && win->draw_content) {
        win->draw_content(win);
    }
}

void sysinfo_handle_mouse(window_t* win, sint32 local_x, sint32 local_y, uint8 buttons) {
    (void)local_x;
    (void)local_y;
    if ((buttons & 0x01) && win && win->draw_content) {
        win->draw_content(win);
    }
}

window_t* sysinfo_create(void) {
    window_t* win = desktop_create_window("System Info", 44, 18, 220, 146);
    if (!win) {
        return NULL;
    }

    win->min_width = 190;
    win->min_height = 126;
    win->draw_content = sysinfo_draw;
    win->handle_key = sysinfo_handle_key;
    win->handle_mouse = sysinfo_handle_mouse;
    get_state(win);
    return win;
}

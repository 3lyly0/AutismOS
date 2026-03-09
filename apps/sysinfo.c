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
    if (!win || !state) {
        return;
    }

    char num[16];
    draw_filled_rect(win->x + 2, win->y + 20, win->width - 4, win->height - 22, COLOR_WHITE);

    draw_text(win->x + 12, win->y + 32, "AutismOS build", COLOR_BLACK);
    draw_text(win->x + 12, win->y + 48, "Display: VGA 320x200", COLOR_BLACK);
    draw_text(win->x + 12, win->y + 64, "Desktop: Pixel GUI", COLOR_BLACK);

    draw_text(win->x + 12, win->y + 88, "Ticks:", COLOR_BLACK);
    uint_to_str((uint32)g_timer_ticks, num);
    draw_text(win->x + 56, win->y + 88, num, COLOR_BLUE);

    draw_text(win->x + 12, win->y + 104, "Status:", COLOR_BLACK);
    draw_text(win->x + 56, win->y + 104, "Running", COLOR_GREEN);

    draw_text(win->x + 12, win->y + 126, "R refreshes", COLOR_DARK_GRAY);
    state->update_counter++;
}

void sysinfo_handle_key(window_t* win, char key) {
    if ((key == 'r' || key == 'R') && win && win->draw_content) {
        win->draw_content(win);
    }
}

window_t* sysinfo_create(void) {
    window_t* win = desktop_create_window("System Info", 54, 24, 200, 140);
    if (!win) {
        return NULL;
    }

    win->draw_content = sysinfo_draw;
    win->handle_key = sysinfo_handle_key;
    get_state(win);
    return win;
}

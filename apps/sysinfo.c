#include "sysinfo.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"

extern volatile uint64 g_timer_ticks;

static sysinfo_state_t sysinfo_states[MAX_WINDOWS];
static uint32 sysinfo_count = 0;

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
    
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

void sysinfo_draw(window_t* win) {
    if (!win) return;
    
    volatile char* video = (volatile char*)0xB8000;
    
    uint32 content_y = win->y + 1;
    uint32 content_x = win->x + 1;
    uint32 content_width = win->width - 2;
    uint32 content_height = win->height - 2;
    
    // Clear content
    for (uint32 row = 0; row < content_height; row++) {
        for (uint32 col = 0; col < content_width; col++) {
            uint32 sy = content_y + row;
            uint32 sx = content_x + col;
            if (sy < 25 && sx < 80) {
                int idx = (sy * 80 + sx) * 2;
                video[idx] = ' ';
                video[idx + 1] = (COLOR_DARK_GRAY << 4) | COLOR_WHITE;
            }
        }
    }
    
    // Title
    draw_text(content_x + 1, content_y, "AUTISMOS v1.0", COLOR_CYAN);
    
    // System info
    draw_text(content_x + 1, content_y + 2, "Architecture:", COLOR_LIGHT_GRAY);
    draw_text(content_x + 15, content_y + 2, "x86 32-bit", COLOR_WHITE);
    
    draw_text(content_x + 1, content_y + 3, "Video Mode:", COLOR_LIGHT_GRAY);
    draw_text(content_x + 15, content_y + 3, "VGA 80x25", COLOR_WHITE);
    
    // Uptime (ticks / 100 = seconds approximately)
    draw_text(content_x + 1, content_y + 5, "Uptime:", COLOR_LIGHT_GRAY);
    uint32 ticks32 = (uint32)g_timer_ticks;  // Use lower 32 bits
    uint32 seconds = ticks32 / 100;
    uint32 mins = seconds / 60;
    uint32 secs = seconds % 60;
    char time_str[20];
    char num[12];
    uint_to_str(mins, num);
    int pos = 0;
    for (int i = 0; num[i]; i++) time_str[pos++] = num[i];
    time_str[pos++] = 'm';
    time_str[pos++] = ' ';
    uint_to_str(secs, num);
    for (int i = 0; num[i]; i++) time_str[pos++] = num[i];
    time_str[pos++] = 's';
    time_str[pos] = '\0';
    draw_text(content_x + 15, content_y + 5, time_str, COLOR_YELLOW);
    
    draw_text(content_x + 1, content_y + 6, "Ticks:", COLOR_LIGHT_GRAY);
    uint_to_str((uint32)g_timer_ticks, num);
    draw_text(content_x + 15, content_y + 6, num, COLOR_WHITE);
    
    // Memory info (basic)
    draw_text(content_x + 1, content_y + 8, "Memory:", COLOR_LIGHT_GRAY);
    draw_text(content_x + 15, content_y + 8, "Paged 4KB", COLOR_WHITE);
    
    // Status
    draw_text(content_x + 1, content_y + 10, "Status:", COLOR_LIGHT_GRAY);
    draw_text(content_x + 15, content_y + 10, "Running", COLOR_LIGHT_GREEN);
    
    // Keyboard shortcut
    draw_text(content_x + 1, content_y + 12, "Press R to refresh", COLOR_DARK_GRAY);
}

void sysinfo_handle_key(window_t* win, char key) {
    if (key == 'r' || key == 'R') {
        sysinfo_draw(win);
    }
}

window_t* sysinfo_create(void) {
    window_t* win = desktop_create_window("System Info", 30, 4, 30, 15);
    if (!win) return NULL;
    
    win->draw_content = sysinfo_draw;
    win->handle_key = sysinfo_handle_key;
    
    if (sysinfo_count < MAX_WINDOWS) {
        sysinfo_state_t* state = &sysinfo_states[sysinfo_count++];
        state->update_counter = 0;
        win->app_data = state;
    }
    
    return win;
}

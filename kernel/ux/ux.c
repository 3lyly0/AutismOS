#include "ux.h"
#include "graphics.h"
#include "boot_animation.h"
#include "string.h"

static ux_state_t g_ux_state;

void ux_init(void) {
    memset(&g_ux_state, 0, sizeof(ux_state_t));
    g_ux_state.boot_complete = 0;
    g_ux_state.silent_mode = 0;
    graphics_init();
}

void ux_show_boot_screen(void) {
    boot_animation_play();
}

void ux_finish_boot(void) {
    g_ux_state.boot_complete = 1;
    g_ux_state.silent_mode = 1;
}

void ux_set_silent_mode(uint8 enabled) {
    g_ux_state.silent_mode = enabled;
}

uint8 ux_is_silent(void) {
    return g_ux_state.silent_mode;
}

void ux_handle_hotkey(char key) {
    if (!g_ux_state.boot_complete) {
        return;
    }

    // Keep Alt+Q as a global shell action placeholder until a power menu exists.
    if (key == 'q' || key == 'Q') {
        graphics_clear_screen(COLOR_BLACK);
        draw_text_scaled(54, 86, "Desktop paused", COLOR_YELLOW, 2);
        graphics_present();
    }
}

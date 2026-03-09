#include "mouse_smooth.h"

static uint8 g_enabled = 0;
static uint8 g_sensitivity = 5;

void mouse_smooth_init(void) {
    g_enabled = 0;
    g_sensitivity = 5;
}

void mouse_smooth_handler(REGISTERS *r) {
    (void)r;
}

int mouse_smooth_get_x(void) {
    return 0;
}

int mouse_smooth_get_y(void) {
    return 0;
}

uint8 mouse_smooth_get_buttons(void) {
    return 0;
}

void mouse_smooth_enable(void) {
    g_enabled = 1;
}

void mouse_smooth_disable(void) {
    g_enabled = 0;
}

uint8 mouse_smooth_is_enabled(void) {
    return g_enabled;
}

void mouse_smooth_set_sensitivity(uint8 level) {
    if (level >= 1 && level <= 10) {
        g_sensitivity = level;
    }
}

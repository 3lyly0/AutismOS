#include "types.h"
#include "mouse_smooth.h"
#include "input_manager.h"

/*
 * Mouse Smoothing Module - AutismOS
 * 
 * Provides smooth mouse movement using exponential moving average.
 * This is now integrated with the input_manager system.
 */

static struct {
    uint8 enabled;
    uint8 sensitivity;
} g_smooth_config;

void mouse_smooth_init(void) {
    g_smooth_config.enabled = 1;
    g_smooth_config.sensitivity = 5;
    
    // Sync with input manager
    input_set_mouse_smoothing(1);
    input_set_mouse_sensitivity(5);
}

void mouse_smooth_handler(REGISTERS* r) {
    (void)r;
    // This is no longer needed - input_manager handles smoothing
}

int mouse_smooth_get_x(void) {
    return input_get_mouse_x();
}

int mouse_smooth_get_y(void) {
    return input_get_mouse_y();
}

uint8 mouse_smooth_get_buttons(void) {
    return (uint8)input_get_mouse_buttons();
}

void mouse_smooth_enable(void) {
    g_smooth_config.enabled = 1;
    input_set_mouse_smoothing(1);
}

void mouse_smooth_disable(void) {
    g_smooth_config.enabled = 0;
    input_set_mouse_smoothing(0);
}

uint8 mouse_smooth_is_enabled(void) {
    return g_smooth_config.enabled;
}

void mouse_smooth_set_sensitivity(uint8 level) {
    if (level >= 1 && level <= 10) {
        g_smooth_config.sensitivity = level;
        input_set_mouse_sensitivity(level);
    }
}
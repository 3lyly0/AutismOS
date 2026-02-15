#include <stdint.h>
#include "kernel.h"
#include "mouse_smooth.h"
#include "video.h"
#include "io_ports.h"
#include "isr.h"
#include "string.h"
#include "desktop.h"

// Smooth mouse state
static int g_smooth_mouse_x = 40;
static int g_smooth_mouse_y = 12;
static uint8 g_smooth_mouse_buttons = 0;
static uint8 g_smooth_enabled = 0;
static uint8 g_smooth_sensitivity = 5;  // Default sensitivity (1-10)

// Acceleration and smoothing
static float g_velocity_x = 0.0f;
static float g_velocity_y = 0.0f;
static float g_position_x = 40.0f;
static float g_position_y = 12.0f;

// Simple float operations (since we don't have full math lib)
static float float_abs(float x) {
    return (x < 0.0f) ? -x : x;
}

static int float_to_int(float x) {
    return (int)x;
}

int mouse_smooth_get_x(void) { 
    return g_smooth_mouse_x; 
}

int mouse_smooth_get_y(void) { 
    return g_smooth_mouse_y; 
}

uint8 mouse_smooth_get_buttons(void) { 
    return g_smooth_mouse_buttons; 
}

uint8 mouse_smooth_is_enabled(void) {
    return g_smooth_enabled;
}

void mouse_smooth_enable(void) {
    g_smooth_enabled = 1;
    // Sync position with current state
    g_position_x = (float)g_smooth_mouse_x;
    g_position_y = (float)g_smooth_mouse_y;
    g_velocity_x = 0.0f;
    g_velocity_y = 0.0f;
}

void mouse_smooth_disable(void) {
    g_smooth_enabled = 0;
}

void mouse_smooth_set_sensitivity(uint8 level) {
    if (level >= 1 && level <= 10) {
        g_smooth_sensitivity = level;
    }
}

void mouse_smooth_handler(REGISTERS *r __attribute__((unused))) {
    static unsigned char mouse_cycle = 0;
    static char mouse_packet[3];

    // Only handle if smooth mode is enabled
    if (!g_smooth_enabled) {
        return;
    }

    mouse_packet[mouse_cycle++] = inportb(0x60);

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        // Get raw movement values
        int x_move = mouse_packet[1];
        int y_move = mouse_packet[2];

        // Sign extend using bit 4 (X) and bit 5 (Y) of byte 0
        if (mouse_packet[0] & 0x10) x_move -= 256;
        if (mouse_packet[0] & 0x20) y_move -= 256;

        // Apply sensitivity scaling (sensitivity ranges from 1-10)
        // Higher sensitivity = faster movement
        float sensitivity_factor = (float)g_smooth_sensitivity / 5.0f;
        
        // Convert raw input to velocity with acceleration
        float raw_vel_x = (float)x_move * sensitivity_factor * 0.4f;
        float raw_vel_y = (float)y_move * sensitivity_factor * 0.4f;

        // Apply acceleration - faster movements get extra boost
        float speed = float_abs(raw_vel_x) + float_abs(raw_vel_y);
        float accel_factor = 1.0f;
        if (speed > 10.0f) {
            accel_factor = 1.0f + (speed - 10.0f) * 0.05f;
            if (accel_factor > 2.5f) accel_factor = 2.5f;
        }
        
        raw_vel_x *= accel_factor;
        raw_vel_y *= accel_factor;

        // Smooth velocity with exponential smoothing (momentum)
        float smoothing = 0.3f;  // Lower = smoother but more lag
        g_velocity_x = g_velocity_x * (1.0f - smoothing) + raw_vel_x * smoothing;
        g_velocity_y = g_velocity_y * (1.0f - smoothing) + raw_vel_y * smoothing;

        // Apply velocity to position
        g_position_x += g_velocity_x;
        g_position_y -= g_velocity_y;  // Inverted Y

        // Convert to integer coordinates
        g_smooth_mouse_x = float_to_int(g_position_x);
        g_smooth_mouse_y = float_to_int(g_position_y);

        // Apply bounds checking
        if (g_smooth_mouse_x < 0) {
            g_smooth_mouse_x = 0;
            g_position_x = 0.0f;
            g_velocity_x = 0.0f;
        }
        if (g_smooth_mouse_x >= SCREEN_WIDTH) {
            g_smooth_mouse_x = SCREEN_WIDTH - 1;
            g_position_x = (float)(SCREEN_WIDTH - 1);
            g_velocity_x = 0.0f;
        }
        if (g_smooth_mouse_y < 0) {
            g_smooth_mouse_y = 0;
            g_position_y = 0.0f;
            g_velocity_y = 0.0f;
        }
        if (g_smooth_mouse_y >= SCREEN_HEIGHT) {
            g_smooth_mouse_y = SCREEN_HEIGHT - 1;
            g_position_y = (float)(SCREEN_HEIGHT - 1);
            g_velocity_y = 0.0f;
        }

        // Update button state
        g_smooth_mouse_buttons = mouse_packet[0] & 0x07;
        
        // Update desktop with smooth mouse movement
        desktop_t* ds = desktop_get_state();
        if (ds && ds->initialized) {
            ds->mouse_x = g_smooth_mouse_x;
            ds->mouse_y = g_smooth_mouse_y;
            ds->mouse_buttons = g_smooth_mouse_buttons;
            
            // Draw mouse immediately from IRQ
            extern void desktop_draw_mouse_direct(void);
            desktop_draw_mouse_direct();
        }
    }
}

static void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inportb(0x64) & 1) == 1) {
                return;
            }
        }
    } else {
        while (timeout--) {
            if ((inportb(0x64) & 2) == 0) {
                return;
            }
        }
    }
}

static void mouse_write(unsigned char data) {
    mouse_wait(1);
    outportb(0x64, 0xD4);
    mouse_wait(1);
    outportb(0x60, data);
}

static unsigned char mouse_read() {
    mouse_wait(0);
    return inportb(0x60);
}

void mouse_smooth_init() {
    // Initialize PS/2 mouse hardware (same as regular mouse)
    mouse_wait(1);
    outportb(0x64, 0xA8);

    mouse_wait(1);
    outportb(0x64, 0x20);
    unsigned char status = mouse_read() | 2;
    mouse_wait(1);
    outportb(0x64, 0x60);
    mouse_wait(1);
    outportb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    // Register smooth mouse handler on IRQ12
    // This will work alongside the regular mouse handler
    isr_register_interrupt_handler(IRQ_BASE + 12, mouse_smooth_handler);
    
    // Enable smooth mode by default
    g_smooth_enabled = 1;
}

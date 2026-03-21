#include "types.h"
#include "mouse.h"
#include "input_manager.h"
#include "io_ports.h"
#include "isr.h"

/*
 * Mouse Driver - AutismOS (Refactored)
 * 
 * Clean PS/2 mouse driver that posts events to the input manager.
 * No direct coupling to desktop or GUI.
 */

// ============================================================================
// PS/2 Mouse Constants
// ============================================================================

#define MOUSE_DATA_PORT    0x60
#define MOUSE_STATUS_PORT  0x64
#define MOUSE_COMMAND_PORT 0x64

// Mouse commands
#define MOUSE_CMD_ENABLE_STREAMING  0xF4
#define MOUSE_CMD_SET_DEFAULTS      0xF6
#define MOUSE_CMD_RESET             0xFF

// Status register bits
#define MOUSE_STATUS_OUTPUT_FULL    0x01
#define MOUSE_STATUS_INPUT_FULL     0x02

// Mouse packet flags
#define MOUSE_PACKET_LEFT_BTN       0x01
#define MOUSE_PACKET_RIGHT_BTN      0x02
#define MOUSE_PACKET_MIDDLE_BTN     0x04
#define MOUSE_PACKET_ALWAYS_1       0x08
#define MOUSE_PACKET_X_SIGN         0x10
#define MOUSE_PACKET_Y_SIGN         0x20
#define MOUSE_PACKET_X_OVERFLOW     0x40
#define MOUSE_PACKET_Y_OVERFLOW     0x80

// ============================================================================
// Internal State
// ============================================================================

static struct {
    uint8 packet[3];
    uint8 packet_index;
    uint8 initialized;
} g_mouse_driver;

// ============================================================================
// PS/2 Low-Level Functions
// ============================================================================

static void mouse_wait_read(void) {
    uint32 timeout = 100000;
    while (timeout--) {
        if ((inportb(MOUSE_STATUS_PORT) & MOUSE_STATUS_OUTPUT_FULL)) {
            return;
        }
    }
}

static void mouse_wait_write(void) {
    uint32 timeout = 100000;
    while (timeout--) {
        if (!(inportb(MOUSE_STATUS_PORT) & MOUSE_STATUS_INPUT_FULL)) {
            return;
        }
    }
}

static void mouse_write_command(uint8 cmd) {
    mouse_wait_write();
    outportb(MOUSE_COMMAND_PORT, 0xD4);  // Tell controller to send to mouse
    mouse_wait_write();
    outportb(MOUSE_DATA_PORT, cmd);
}

static uint8 mouse_read_data(void) {
    mouse_wait_read();
    return inportb(MOUSE_DATA_PORT);
}

static void mouse_send_command(uint8 cmd) {
    mouse_write_command(cmd);
    mouse_read_data();  // Acknowledge
}

// ============================================================================
// Interrupt Handler
// ============================================================================

void mouse_handler(REGISTERS* r __attribute__((unused))) {
    if (!g_mouse_driver.initialized) {
        return;
    }
    
    // Read data byte
    uint8 data = inportb(MOUSE_DATA_PORT);
    
    // Accumulate packet
    g_mouse_driver.packet[g_mouse_driver.packet_index++] = data;
    
    // Wait for complete 3-byte packet
    if (g_mouse_driver.packet_index < 3) {
        return;
    }
    g_mouse_driver.packet_index = 0;
    
    // Validate packet: bit 3 of first byte should always be set
    if (!(g_mouse_driver.packet[0] & MOUSE_PACKET_ALWAYS_1)) {
        return;
    }
    
    // Ignore overflow packets (mouse moved too fast)
    if (g_mouse_driver.packet[0] & (MOUSE_PACKET_X_OVERFLOW | MOUSE_PACKET_Y_OVERFLOW)) {
        return;
    }
    
    // Extract button state
    mouse_button_t buttons = MOUSE_BUTTON_NONE;
    if (g_mouse_driver.packet[0] & MOUSE_PACKET_LEFT_BTN) {
        buttons |= MOUSE_BUTTON_LEFT;
    }
    if (g_mouse_driver.packet[0] & MOUSE_PACKET_RIGHT_BTN) {
        buttons |= MOUSE_BUTTON_RIGHT;
    }
    if (g_mouse_driver.packet[0] & MOUSE_PACKET_MIDDLE_BTN) {
        buttons |= MOUSE_BUTTON_MIDDLE;
    }
    
    // Extract movement deltas (signed 9-bit values)
    sint32 dx = g_mouse_driver.packet[1];
    sint32 dy = g_mouse_driver.packet[2];
    
    // Apply sign extension
    if (g_mouse_driver.packet[0] & MOUSE_PACKET_X_SIGN) {
        dx |= 0xFFFFFF00;  // Sign extend to 32-bit
    }
    if (g_mouse_driver.packet[0] & MOUSE_PACKET_Y_SIGN) {
        dy |= 0xFFFFFF00;
    }
    
    // Clamp large movements (prevents jumps)
    if (dx > 127) dx = 127;
    if (dx < -127) dx = -127;
    if (dy > 127) dy = 127;
    if (dy < -127) dy = -127;
    
    // Get previous button state for change detection
    static mouse_button_t prev_buttons = MOUSE_BUTTON_NONE;
    
    // Detect button changes and post events
    mouse_button_t pressed = buttons & ~prev_buttons;
    mouse_button_t released = prev_buttons & ~buttons;
    
    if (pressed & MOUSE_BUTTON_LEFT) {
        input_post_mouse_button(MOUSE_BUTTON_LEFT, 1);
    }
    if (released & MOUSE_BUTTON_LEFT) {
        input_post_mouse_button(MOUSE_BUTTON_LEFT, 0);
    }
    if (pressed & MOUSE_BUTTON_RIGHT) {
        input_post_mouse_button(MOUSE_BUTTON_RIGHT, 1);
    }
    if (released & MOUSE_BUTTON_RIGHT) {
        input_post_mouse_button(MOUSE_BUTTON_RIGHT, 0);
    }
    if (pressed & MOUSE_BUTTON_MIDDLE) {
        input_post_mouse_button(MOUSE_BUTTON_MIDDLE, 1);
    }
    if (released & MOUSE_BUTTON_MIDDLE) {
        input_post_mouse_button(MOUSE_BUTTON_MIDDLE, 0);
    }
    
    prev_buttons = buttons;
    
    // Post movement event (only if there's actual movement)
    if (dx != 0 || dy != 0) {
        input_post_mouse_move(dx, dy);
    }
}

// ============================================================================
// Legacy API (for backward compatibility)
// ============================================================================

int mouse_get_x(void) {
    return input_get_mouse_x();
}

int mouse_get_y(void) {
    return input_get_mouse_y();
}

uint8 mouse_get_buttons(void) {
    return (uint8)input_get_mouse_buttons();
}

void draw_pointer(int x, int y) {
    // Deprecated: cursor drawing is handled by desktop/UI layer
    (void)x;
    (void)y;
}

void clear_pointer(int x, int y) {
    // Deprecated: cursor drawing is handled by desktop/UI layer
    (void)x;
    (void)y;
}

// ============================================================================
// Initialization
// ============================================================================

void mouse_init(void) {
    // Reset state
    g_mouse_driver.packet_index = 0;
    g_mouse_driver.initialized = 0;
    
    // Enable auxiliary device (mouse)
    mouse_wait_write();
    outportb(MOUSE_COMMAND_PORT, 0xA8);
    
    // Enable interrupts
    mouse_wait_write();
    outportb(MOUSE_COMMAND_PORT, 0x20);
    
    uint8 status = mouse_read_data();
    status |= 0x02;  // Enable IRQ12
    status &= ~0x20; // Clear disable clock
    
    mouse_wait_write();
    outportb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait_write();
    outportb(MOUSE_DATA_PORT, status);
    
    // Set defaults
    mouse_send_command(MOUSE_CMD_SET_DEFAULTS);
    
    // Enable streaming
    mouse_send_command(MOUSE_CMD_ENABLE_STREAMING);
    
    // Register interrupt handler (IRQ12 = 44 = 32 + 12)
    isr_register_interrupt_handler(IRQ_BASE + 12, mouse_handler);
    
    g_mouse_driver.initialized = 1;
}
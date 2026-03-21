#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "types.h"
#include "input_events.h"

/*
 * Input Manager - AutismOS
 * 
 * Central hub for all input events. Provides:
 * - Event queue for polling
 * - Listener system for callbacks
 * - Unified mouse/keyboard state
 */

// ============================================================================
// Configuration
// ============================================================================

#define MAX_LISTENERS 8

// ============================================================================
// Input Manager State
// ============================================================================

typedef struct input_manager {
    // Event queue
    input_queue_t queue;
    
    // Listeners
    input_listener_t listeners[MAX_LISTENERS];
    uint32 listener_count;
    
    // Current state
    mouse_state_t mouse;
    input_modifiers_t modifiers;
    
    // Settings
    uint8 mouse_sensitivity;    // 1-10, default 5
    uint8 mouse_acceleration;   // 0-3, 0 = off
    uint8 mouse_smoothing;      // 0-3, 0 = off
    
    // Smoothing state
    sint32 mouse_velocity_x;
    sint32 mouse_velocity_y;
    
    // Timestamp
    uint32 tick_count;
} input_manager_t;

// ============================================================================
// Initialization
// ============================================================================

void input_manager_init(void);

// ============================================================================
// Event Posting (called by drivers)
// ============================================================================

void input_post_event(const input_event_t* event);

// Convenience functions
void input_post_mouse_move(sint32 dx, sint32 dy);
void input_post_mouse_button(mouse_button_t button, uint8 pressed);
void input_post_mouse_wheel(sint32 delta);
void input_post_key(keycode_t keycode, uint8 pressed, char character);

// ============================================================================
// Event Consumption (called by applications)
// ============================================================================

// Poll for next event (non-blocking)
uint8 input_poll_event(input_event_t* out_event);

// Check if queue has events
uint8 input_has_events(void);

// Clear all pending events
void input_flush_events(void);

// ============================================================================
// Listeners
// ============================================================================

// Register a listener (returns -1 on failure)
int input_add_listener(const char* name, input_listener_fn callback, void* user_data, uint8 priority);

// Remove a listener by name
void input_remove_listener(const char* name);

// ============================================================================
// State Queries
// ============================================================================

const mouse_state_t* input_get_mouse_state(void);
const input_modifiers_t* input_get_modifiers(void);

sint32 input_get_mouse_x(void);
sint32 input_get_mouse_y(void);
mouse_button_t input_get_mouse_buttons(void);
uint8 input_mouse_button_down(mouse_button_t button);

// ============================================================================
// Mouse Control
// ============================================================================

void input_set_mouse_position(sint32 x, sint32 y);
void input_set_mouse_visible(uint8 visible);
void input_confine_mouse(sint32 x, sint32 y, sint32 w, sint32 h);
void input_unconfine_mouse(void);

// ============================================================================
// Settings
// ============================================================================

void input_set_mouse_sensitivity(uint8 level);  // 1-10
void input_set_mouse_acceleration(uint8 level); // 0-3
void input_set_mouse_smoothing(uint8 level);    // 0-3

// ============================================================================
// Tick Update (called from timer)
// ============================================================================

void input_tick(void);

#endif // INPUT_MANAGER_H
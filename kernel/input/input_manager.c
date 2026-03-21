#include "input_manager.h"
#include "string.h"
#include "video.h"

/*
 * Input Manager Implementation - AutismOS
 * 
 * Central hub for all input events.
 */

static input_manager_t g_input;

// ============================================================================
// Initialization
// ============================================================================

void input_manager_init(void) {
    memset(&g_input, 0, sizeof(input_manager_t));
    
    // Initialize mouse at center of screen
    g_input.mouse.x = SCREEN_WIDTH / 2;
    g_input.mouse.y = SCREEN_HEIGHT / 2;
    g_input.mouse.visible = 1;
    
    // Default settings
    g_input.mouse_sensitivity = 5;
    g_input.mouse_acceleration = 1;
    g_input.mouse_smoothing = 1;
}

// ============================================================================
// Internal: Apply mouse acceleration
// ============================================================================

static sint32 apply_acceleration(sint32 delta) {
    if (g_input.mouse_acceleration == 0) {
        return delta;
    }
    
    // Simple acceleration: multiply by (1 + |delta| / threshold)
    sint32 accel_factor = 1 + (delta * delta) / (64 * g_input.mouse_acceleration);
    
    if (delta > 0) {
        return delta * accel_factor / 8;
    } else {
        return delta * accel_factor / 8;
    }
}

// ============================================================================
// Internal: Apply mouse smoothing (exponential moving average)
// ============================================================================

static void apply_smoothing(sint32* dx, sint32* dy) {
    if (g_input.mouse_smoothing == 0) {
        return;
    }
    
    // Smoothing factor (higher = more smoothing)
    sint32 factor = g_input.mouse_smoothing * 2;
    
    // Update velocity
    g_input.mouse_velocity_x = (g_input.mouse_velocity_x * factor + *dx) / (factor + 1);
    g_input.mouse_velocity_y = (g_input.mouse_velocity_y * factor + *dy) / (factor + 1);
    
    // Blend current input with smoothed velocity
    *dx = (*dx + g_input.mouse_velocity_x * 2) / 3;
    *dy = (*dy + g_input.mouse_velocity_y * 2) / 3;
}

// ============================================================================
// Internal: Clamp mouse to screen/confine region
// ============================================================================

static void clamp_mouse_position(void) {
    sint32 min_x = 0;
    sint32 min_y = 0;
    sint32 max_x = SCREEN_WIDTH - 10;  // Leave some margin for cursor
    sint32 max_y = SCREEN_HEIGHT - 16;
    
    if (g_input.mouse.confined) {
        min_x = g_input.mouse.confine_x;
        min_y = g_input.mouse.confine_y;
        max_x = g_input.mouse.confine_x + g_input.mouse.confine_w - 10;
        max_y = g_input.mouse.confine_y + g_input.mouse.confine_h - 16;
    }
    
    if (g_input.mouse.x < min_x) g_input.mouse.x = min_x;
    if (g_input.mouse.y < min_y) g_input.mouse.y = min_y;
    if (g_input.mouse.x > max_x) g_input.mouse.x = max_x;
    if (g_input.mouse.y > max_y) g_input.mouse.y = max_y;
}

// ============================================================================
// Event Posting
// ============================================================================

void input_post_event(const input_event_t* event) {
    if (!event) return;
    
    input_queue_t* q = &g_input.queue;
    
    // Drop event if queue is full
    if (q->count >= INPUT_QUEUE_SIZE) {
        return;
    }
    
    // Add to queue
    q->events[q->tail] = *event;
    q->tail = (q->tail + 1) % INPUT_QUEUE_SIZE;
    q->count++;
    
    // Update internal state based on event
    switch (event->type) {
        case INPUT_EVENT_MOUSE_MOVE: {
            sint32 dx = event->data.mouse.dx;
            sint32 dy = event->data.mouse.dy;
            
            // Apply sensitivity (multiply by sensitivity, divide by 5)
            dx = dx * (sint32)g_input.mouse_sensitivity / 5;
            dy = dy * (sint32)g_input.mouse_sensitivity / 5;
            
            // Apply acceleration
            dx = apply_acceleration(dx);
            dy = apply_acceleration(dy);
            
            // Apply smoothing
            apply_smoothing(&dx, &dy);
            
            // Update position (Y is inverted for typical mouse movement)
            g_input.mouse.x += dx;
            g_input.mouse.y -= dy;
            
            clamp_mouse_position();
            
            g_input.mouse.buttons = event->data.mouse.buttons;
            break;
        }
        
        case INPUT_EVENT_MOUSE_PRESS:
        case INPUT_EVENT_MOUSE_RELEASE:
            g_input.mouse.buttons = event->data.mouse.buttons;
            g_input.mouse.x = event->data.mouse.x;
            g_input.mouse.y = event->data.mouse.y;
            break;
            
        case INPUT_EVENT_KEY_PRESS:
        case INPUT_EVENT_KEY_RELEASE:
            // Update modifiers
            if (event->data.key.keycode == KEY_SHIFT) {
                g_input.modifiers.shift = (event->type == INPUT_EVENT_KEY_PRESS);
            } else if (event->data.key.keycode == KEY_CTRL) {
                g_input.modifiers.ctrl = (event->type == INPUT_EVENT_KEY_PRESS);
            } else if (event->data.key.keycode == KEY_ALT) {
                g_input.modifiers.alt = (event->type == INPUT_EVENT_KEY_PRESS);
            } else if (event->data.key.keycode == KEY_CAPSLOCK) {
                if (event->type == INPUT_EVENT_KEY_PRESS) {
                    g_input.modifiers.capslock = !g_input.modifiers.capslock;
                }
            }
            break;
            
        default:
            break;
    }
    
    // Notify listeners
    for (uint32 i = 0; i < g_input.listener_count; i++) {
        input_listener_t* listener = &g_input.listeners[i];
        if (listener->active && listener->callback) {
            listener->callback(event, listener->user_data);
        }
    }
}

// ============================================================================
// Convenience Posting Functions
// ============================================================================

void input_post_mouse_move(sint32 dx, sint32 dy) {
    input_event_t event = {0};
    event.type = INPUT_EVENT_MOUSE_MOVE;
    event.timestamp = g_input.tick_count;
    event.data.mouse.x = g_input.mouse.x;
    event.data.mouse.y = g_input.mouse.y;
    event.data.mouse.dx = dx;
    event.data.mouse.dy = dy;
    event.data.mouse.buttons = g_input.mouse.buttons;
    input_post_event(&event);
}

void input_post_mouse_button(mouse_button_t button, uint8 pressed) {
    input_event_t event = {0};
    event.type = pressed ? INPUT_EVENT_MOUSE_PRESS : INPUT_EVENT_MOUSE_RELEASE;
    event.timestamp = g_input.tick_count;
    event.data.mouse.x = g_input.mouse.x;
    event.data.mouse.y = g_input.mouse.y;
    event.data.mouse.buttons = g_input.mouse.buttons;
    event.data.mouse.button_changed = button;
    
    // Update button state
    if (pressed) {
        event.data.mouse.buttons |= button;
    } else {
        event.data.mouse.buttons &= ~button;
    }
    
    input_post_event(&event);
}

void input_post_mouse_wheel(sint32 delta) {
    input_event_t event = {0};
    event.type = INPUT_EVENT_MOUSE_WHEEL;
    event.timestamp = g_input.tick_count;
    event.data.mouse.x = g_input.mouse.x;
    event.data.mouse.y = g_input.mouse.y;
    event.data.mouse.buttons = g_input.mouse.buttons;
    event.data.mouse.wheel_delta = delta;
    input_post_event(&event);
}

void input_post_key(keycode_t keycode, uint8 pressed, char character) {
    input_event_t event = {0};
    event.type = pressed ? INPUT_EVENT_KEY_PRESS : INPUT_EVENT_KEY_RELEASE;
    event.timestamp = g_input.tick_count;
    event.data.key.keycode = keycode;
    event.data.key.character = character;
    event.data.key.shift = g_input.modifiers.shift;
    event.data.key.ctrl = g_input.modifiers.ctrl;
    event.data.key.alt = g_input.modifiers.alt;
    event.data.key.capslock = g_input.modifiers.capslock;
    input_post_event(&event);
}

// ============================================================================
// Event Consumption
// ============================================================================

uint8 input_poll_event(input_event_t* out_event) {
    if (!out_event) return 0;
    
    input_queue_t* q = &g_input.queue;
    
    if (q->count == 0) return 0;
    
    *out_event = q->events[q->head];
    q->head = (q->head + 1) % INPUT_QUEUE_SIZE;
    q->count--;
    
    return 1;
}

uint8 input_has_events(void) {
    return g_input.queue.count > 0;
}

void input_flush_events(void) {
    g_input.queue.head = 0;
    g_input.queue.tail = 0;
    g_input.queue.count = 0;
}

// ============================================================================
// Listeners
// ============================================================================

int input_add_listener(const char* name, input_listener_fn callback, void* user_data, uint8 priority) {
    if (g_input.listener_count >= MAX_LISTENERS) {
        return -1;
    }
    
    if (!name || !callback) {
        return -1;
    }
    
    // Find insertion point (keep sorted by priority, descending)
    uint32 insert_at = g_input.listener_count;
    for (uint32 i = 0; i < g_input.listener_count; i++) {
        if (priority > g_input.listeners[i].priority) {
            insert_at = i;
            break;
        }
    }
    
    // Shift listeners to make room
    for (uint32 i = g_input.listener_count; i > insert_at; i--) {
        g_input.listeners[i] = g_input.listeners[i - 1];
    }
    
    // Add new listener
    input_listener_t* listener = &g_input.listeners[insert_at];
    strncpy(listener->name, name, INPUT_LISTENER_NAME_LEN - 1);
    listener->name[INPUT_LISTENER_NAME_LEN - 1] = '\0';
    listener->callback = callback;
    listener->user_data = user_data;
    listener->active = 1;
    listener->priority = priority;
    
    g_input.listener_count++;
    
    return (int)insert_at;
}

void input_remove_listener(const char* name) {
    if (!name) return;
    
    for (uint32 i = 0; i < g_input.listener_count; i++) {
        if (strncmp(g_input.listeners[i].name, name, INPUT_LISTENER_NAME_LEN) == 0) {
            // Shift remaining listeners
            for (uint32 j = i; j < g_input.listener_count - 1; j++) {
                g_input.listeners[j] = g_input.listeners[j + 1];
            }
            g_input.listener_count--;
            return;
        }
    }
}

// ============================================================================
// State Queries
// ============================================================================

const mouse_state_t* input_get_mouse_state(void) {
    return &g_input.mouse;
}

const input_modifiers_t* input_get_modifiers(void) {
    return &g_input.modifiers;
}

sint32 input_get_mouse_x(void) {
    return g_input.mouse.x;
}

sint32 input_get_mouse_y(void) {
    return g_input.mouse.y;
}

mouse_button_t input_get_mouse_buttons(void) {
    return g_input.mouse.buttons;
}

uint8 input_mouse_button_down(mouse_button_t button) {
    return (g_input.mouse.buttons & button) != 0;
}

// ============================================================================
// Mouse Control
// ============================================================================

void input_set_mouse_position(sint32 x, sint32 y) {
    g_input.mouse.x = x;
    g_input.mouse.y = y;
    clamp_mouse_position();
}

void input_set_mouse_visible(uint8 visible) {
    g_input.mouse.visible = visible;
}

void input_confine_mouse(sint32 x, sint32 y, sint32 w, sint32 h) {
    g_input.mouse.confined = 1;
    g_input.mouse.confine_x = x;
    g_input.mouse.confine_y = y;
    g_input.mouse.confine_w = w;
    g_input.mouse.confine_h = h;
    clamp_mouse_position();
}

void input_unconfine_mouse(void) {
    g_input.mouse.confined = 0;
}

// ============================================================================
// Settings
// ============================================================================

void input_set_mouse_sensitivity(uint8 level) {
    if (level >= 1 && level <= 10) {
        g_input.mouse_sensitivity = level;
    }
}

void input_set_mouse_acceleration(uint8 level) {
    if (level <= 3) {
        g_input.mouse_acceleration = level;
    }
}

void input_set_mouse_smoothing(uint8 level) {
    if (level <= 3) {
        g_input.mouse_smoothing = level;
    }
}

// ============================================================================
// Tick Update
// ============================================================================

void input_tick(void) {
    g_input.tick_count++;
    
    // Decay mouse velocity when no movement
    g_input.mouse_velocity_x = (g_input.mouse_velocity_x * 7) / 8;
    g_input.mouse_velocity_y = (g_input.mouse_velocity_y * 7) / 8;
}
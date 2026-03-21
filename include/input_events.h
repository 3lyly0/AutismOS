#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H

#include "types.h"

/*
 * Input Event System - AutismOS
 * 
 * A clean, decoupled input system that uses event queues
 * instead of direct function calls.
 */

// ============================================================================
// Event Types
// ============================================================================

typedef enum {
    INPUT_EVENT_NONE = 0,
    
    // Keyboard events
    INPUT_EVENT_KEY_PRESS,
    INPUT_EVENT_KEY_RELEASE,
    INPUT_EVENT_KEY_CHAR,       // Translated character
    
    // Mouse events
    INPUT_EVENT_MOUSE_MOVE,
    INPUT_EVENT_MOUSE_PRESS,
    INPUT_EVENT_MOUSE_RELEASE,
    INPUT_EVENT_MOUSE_WHEEL,
    
    // Touch events (future)
    INPUT_EVENT_TOUCH_BEGIN,
    INPUT_EVENT_TOUCH_MOVE,
    INPUT_EVENT_TOUCH_END,
} input_event_type_t;

// ============================================================================
// Key Codes
// ============================================================================

typedef enum {
    KEY_NONE = 0,
    
    // Special keys
    KEY_ESCAPE = 0x01,
    KEY_1 = 0x02, KEY_2 = 0x03, KEY_3 = 0x04, KEY_4 = 0x05,
    KEY_5 = 0x06, KEY_6 = 0x07, KEY_7 = 0x08, KEY_8 = 0x09,
    KEY_9 = 0x0A, KEY_0 = 0x0B,
    
    KEY_BACKSPACE = 0x0E,
    KEY_TAB = 0x0F,
    KEY_ENTER = 0x1C,
    KEY_CTRL = 0x1D,
    KEY_SHIFT = 0x2A,
    KEY_ALT = 0x38,
    
    KEY_LEFT = 0x4B,
    KEY_RIGHT = 0x4D,
    KEY_UP = 0x48,
    KEY_DOWN = 0x50,
    
    KEY_F1 = 0x3B, KEY_F2 = 0x3C, KEY_F3 = 0x3D, KEY_F4 = 0x3E,
    KEY_F5 = 0x3F, KEY_F6 = 0x40, KEY_F7 = 0x41, KEY_F8 = 0x42,
    KEY_F9 = 0x43, KEY_F10 = 0x44, KEY_F11 = 0x57, KEY_F12 = 0x58,
    
    KEY_HOME = 0x47,
    KEY_END = 0x4F,
    KEY_PAGEUP = 0x49,
    KEY_PAGEDOWN = 0x51,
    KEY_INSERT = 0x52,
    KEY_DELETE = 0x53,
    
    KEY_CAPSLOCK = 0x3A,
    KEY_NUMLOCK = 0x45,
    KEY_SCROLLLOCK = 0x46,
} keycode_t;

// ============================================================================
// Mouse Button Flags
// ============================================================================

typedef enum {
    MOUSE_BUTTON_NONE   = 0,
    MOUSE_BUTTON_LEFT   = (1 << 0),
    MOUSE_BUTTON_RIGHT  = (1 << 1),
    MOUSE_BUTTON_MIDDLE = (1 << 2),
} mouse_button_t;

// ============================================================================
// Input Event Structure
// ============================================================================

typedef struct input_event {
    input_event_type_t type;
    uint32 timestamp;          // Ticks since boot
    
    // Union for different event data
    union {
        // Keyboard
        struct {
            keycode_t keycode;
            char character;      // Translated char (if any)
            uint8 shift: 1;
            uint8 ctrl: 1;
            uint8 alt: 1;
            uint8 capslock: 1;
        } key;
        
        // Mouse
        struct {
            sint32 x;
            sint32 y;
            sint32 dx;           // Delta x (relative movement)
            sint32 dy;
            mouse_button_t buttons;
            mouse_button_t button_changed;  // Which button triggered this event
            sint32 wheel_delta;  // Scroll wheel
        } mouse;
    } data;
} input_event_t;

// ============================================================================
// Event Listener (callback-based)
// ============================================================================

#define INPUT_LISTENER_NAME_LEN 16

typedef void (*input_listener_fn)(const input_event_t* event, void* user_data);

typedef struct input_listener {
    char name[INPUT_LISTENER_NAME_LEN];
    input_listener_fn callback;
    void* user_data;
    uint8 active;
    uint8 priority;          // Higher = called first
} input_listener_t;

// ============================================================================
// Event Queue (for polling-based consumption)
// ============================================================================

#define INPUT_QUEUE_SIZE 64

typedef struct input_queue {
    input_event_t events[INPUT_QUEUE_SIZE];
    uint32 head;
    uint32 tail;
    uint32 count;
} input_queue_t;

// ============================================================================
// Modifier State
// ============================================================================

typedef struct input_modifiers {
    uint8 shift: 1;
    uint8 ctrl: 1;
    uint8 alt: 1;
    uint8 capslock: 1;
    uint8 numlock: 1;
} input_modifiers_t;

// ============================================================================
// Mouse State
// ============================================================================

typedef struct mouse_state {
    sint32 x;
    sint32 y;
    mouse_button_t buttons;
    uint8 visible;
    uint8 confined;          // Is mouse confined to a region?
    sint32 confine_x;
    sint32 confine_y;
    sint32 confine_w;
    sint32 confine_h;
} mouse_state_t;

#endif // INPUT_EVENTS_H
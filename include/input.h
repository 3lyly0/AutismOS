#ifndef INPUT_H
#define INPUT_H

#include "types.h"

// Input event types
#define INPUT_EVENT_KEY_DOWN   1
#define INPUT_EVENT_KEY_UP     2
#define INPUT_EVENT_MOUSE_MOVE 3
#define INPUT_EVENT_MOUSE_DOWN 4
#define INPUT_EVENT_MOUSE_UP   5

// Input event structure
typedef struct input_event {
    uint32 type;        // Event type (key, mouse, etc.)
    uint32 code;        // Key code or mouse button
    uint32 x;           // Mouse X coordinate (for mouse events)
    uint32 y;           // Mouse Y coordinate (for mouse events)
} input_event_t;

// Input subsystem functions
void input_init(void);
void input_send_key_event(uint32 type, uint32 keycode);
void input_send_mouse_event(uint32 type, uint32 button, uint32 x, uint32 y);

#endif

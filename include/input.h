#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/*
 * Legacy Input Subsystem - AutismOS
 * 
 * This provides backward compatibility with the old input system.
 * New code should use input_manager.h instead.
 */

// Legacy event type codes (for IPC messages)
#define INPUT_EVENT_KEY_DOWN   1
#define INPUT_EVENT_KEY_UP     2
#define INPUT_EVENT_MOUSE_DOWN 4
#define INPUT_EVENT_MOUSE_UP   5

// Legacy input functions (for backward compatibility)
void input_init(void);
void input_send_key_event(uint32 type, uint32 keycode);
void input_send_mouse_event(uint32 type, uint32 button, uint32 x, uint32 y);

#endif // INPUT_H
#ifndef MOUSE_SMOOTH_H
#define MOUSE_SMOOTH_H

#include "types.h"
#include "isr.h"

// Initialize smooth mouse driver
void mouse_smooth_init(void);

// Smooth mouse interrupt handler
void mouse_smooth_handler(REGISTERS *r);

// Get smooth mouse state
int mouse_smooth_get_x(void);
int mouse_smooth_get_y(void);
uint8 mouse_smooth_get_buttons(void);

// Enable/disable smooth mouse mode
void mouse_smooth_enable(void);
void mouse_smooth_disable(void);
uint8 mouse_smooth_is_enabled(void);

// Adjust sensitivity (1-10, default 5)
void mouse_smooth_set_sensitivity(uint8 level);

#endif

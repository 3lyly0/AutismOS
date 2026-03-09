#ifndef UX_H
#define UX_H

#include "types.h"

// Minimal desktop UX state used during boot and global shell hotkeys.
typedef struct {
    uint8 boot_complete;
    uint8 silent_mode;
} ux_state_t;

void ux_init(void);
void ux_show_boot_screen(void);
void ux_finish_boot(void);

void ux_set_silent_mode(uint8 enabled);
uint8 ux_is_silent(void);
void ux_handle_hotkey(char key);

#endif

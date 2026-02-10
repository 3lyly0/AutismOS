#ifndef UX_H
#define UX_H

#include "types.h"
#include "ui.h"

// UX (User Experience) Kernel
// Step 8: Makes the OS feel real by managing:
// - Clean boot surface
// - Input control loop
// - Focus & ownership
// - Minimal persistence
// Step 9: Interactive Graphics UI
// - Graphical interface with textbox
// - In-page interaction
// - Visual feedback

// System state persistence (survives across process restarts)
typedef struct {
    char last_url[256];       // Last URL visited
    uint32 active_process;    // Currently focused process PID
    uint8 boot_complete;      // Boot sequence finished flag
    uint8 silent_mode;        // Hide debug logs
    textbox_t url_textbox;    // Step 9: URL input textbox
    uint8 ui_initialized;     // Step 9: UI is ready
} ux_state_t;

// Initialize UX kernel
void ux_init(void);

// Boot sequence management
void ux_show_boot_screen(void);
void ux_finish_boot(void);

// Logging control
void ux_set_silent_mode(uint8 enabled);
uint8 ux_is_silent(void);

// Persistence
void ux_save_url(const char* url);
const char* ux_get_last_url(void);
void ux_set_active_process(uint32 pid);
uint32 ux_get_active_process(void);

// Control loop (keyboard shortcuts)
void ux_handle_hotkey(char key);

// Step 9: UI management
void ux_init_ui(void);
void ux_show_ready_screen(void);
void ux_handle_key_input(char ch);
void ux_handle_enter_key(void);
textbox_t* ux_get_url_textbox(void);
void ux_update_caret(void);

#endif

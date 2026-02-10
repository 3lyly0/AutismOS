#include "ux.h"
#include "video.h"
#include "string.h"

// Global UX state (persists in kernel memory)
static ux_state_t g_ux_state;

// Initialize UX kernel
void ux_init(void) {
    // Clear state
    memset(&g_ux_state, 0, sizeof(ux_state_t));
    
    // Set defaults
    strcpy(g_ux_state.last_url, "http://example.com/");
    g_ux_state.active_process = 1; // Browser process by default
    g_ux_state.boot_complete = 0;
    g_ux_state.silent_mode = 0;
}

// Show clean boot screen
void ux_show_boot_screen(void) {
    clear_screen();
    print("\n");
    print("  █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗\n");
    print(" ██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝\n");
    print(" ███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗\n");
    print(" ██╔══██║██║   ██║   ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║\n");
    print(" ██║  ██║╚██████╔╝   ██║   ██║███████║██║ ╚═╝ ██║╚██████╔╝███████║\n");
    print(" ╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝\n");
    print("\n");
    print("                    Browser-First Operating System\n");
    print("\n");
    print("  Booting...\n");
}

// Finish boot sequence and transition to main UI
void ux_finish_boot(void) {
    g_ux_state.boot_complete = 1;
    g_ux_state.silent_mode = 1;
    
    clear_screen();
    print("\n");
    print("  AutismOS - Ready\n");
    print("  ═══════════════════════════════════════════════════════════════════\n");
    print("\n");
    print("  Active: Browser");
    print("\n");
    print("  URL: ");
    print(g_ux_state.last_url);
    print("\n\n");
    print("  Shortcuts:\n");
    print("    Alt+B - Focus Browser\n");
    print("    Alt+Q - Quit Application\n");
    print("\n");
    print("  ═══════════════════════════════════════════════════════════════════\n");
}

// Silent mode control
void ux_set_silent_mode(uint8 enabled) {
    g_ux_state.silent_mode = enabled;
}

uint8 ux_is_silent(void) {
    return g_ux_state.silent_mode;
}

// Persistence - URL
void ux_save_url(const char* url) {
    if (url) {
        strncpy(g_ux_state.last_url, url, sizeof(g_ux_state.last_url) - 1);
        g_ux_state.last_url[sizeof(g_ux_state.last_url) - 1] = '\0';
    }
}

const char* ux_get_last_url(void) {
    return g_ux_state.last_url;
}

// Persistence - Active process
void ux_set_active_process(uint32 pid) {
    g_ux_state.active_process = pid;
}

uint32 ux_get_active_process(void) {
    return g_ux_state.active_process;
}

// Handle keyboard shortcuts
void ux_handle_hotkey(char key) {
    // Alt+B (0x42) - Focus Browser
    if (key == 'b' || key == 'B') {
        if (g_ux_state.boot_complete) {
            clear_screen();
            print("\n  [Browser Activated]\n\n");
            ux_set_active_process(1);
        }
    }
    // Alt+Q (0x51) - Quit
    else if (key == 'q' || key == 'Q') {
        if (g_ux_state.boot_complete) {
            clear_screen();
            print("\n  [Quitting Application]\n\n");
        }
    }
}

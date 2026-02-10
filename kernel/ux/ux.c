#include "ux.h"
#include "video.h"
#include "graphics.h"
#include "ui.h"
#include "string.h"

// Global UX state (persists in kernel memory)
static ux_state_t g_ux_state;

// Initialize UX kernel
void ux_init(void) {
    // Clear state
    memset(&g_ux_state, 0, sizeof(ux_state_t));
    
    // Set defaults
    strncpy(g_ux_state.last_ip, "192.168.1.1", sizeof(g_ux_state.last_ip) - 1);
    g_ux_state.last_ip[sizeof(g_ux_state.last_ip) - 1] = '\0';
    g_ux_state.active_process = 1; // Browser process by default
    g_ux_state.boot_complete = 0;
    g_ux_state.silent_mode = 0;
    g_ux_state.ui_initialized = 0;
    
    // Initialize graphics and UI subsystems
    graphics_init();
    ui_init();
}

// Show clean boot screen
void ux_show_boot_screen(void) {
    graphics_clear_screen(COLOR_BLACK);
    draw_text(2, 2, "  █████╗ ██╗   ██╗████████╗██╗███████╗███╗   ███╗ ██████╗ ███████╗", COLOR_LIGHT_CYAN);
    draw_text(2, 3, " ██╔══██╗██║   ██║╚══██╔══╝██║██╔════╝████╗ ████║██╔═══██╗██╔════╝", COLOR_LIGHT_CYAN);
    draw_text(2, 4, " ███████║██║   ██║   ██║   ██║███████╗██╔████╔██║██║   ██║███████╗", COLOR_LIGHT_CYAN);
    draw_text(2, 5, " ██╔══██║██║   ██║   ██║   ██║╚════██║██║╚██╔╝██║██║   ██║╚════██║", COLOR_LIGHT_CYAN);
    draw_text(2, 6, " ██║  ██║╚██████╔╝   ██║   ██║███████║██║ ╚═╝ ██║╚██████╔╝███████║", COLOR_LIGHT_CYAN);
    draw_text(2, 7, " ╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝", COLOR_LIGHT_CYAN);
    draw_text(20, 9, "Network Ping Utility", COLOR_WHITE);
    draw_text(2, 11, "  Booting...", COLOR_YELLOW);
}

// Initialize UI components (Step 9)
void ux_init_ui(void) {
    if (g_ux_state.ui_initialized) {
        return;
    }
    
    // Initialize IP textbox (adjusted position for "IP Address:" label)
    textbox_init(&g_ux_state.url_textbox, 14, 5, 56, 3);
    
    // Pre-fill with last IP
    if (g_ux_state.last_ip[0]) {
        strncpy(g_ux_state.url_textbox.buffer, g_ux_state.last_ip, TEXTBOX_MAX_LEN - 1);
        g_ux_state.url_textbox.cursor_pos = strlen(g_ux_state.url_textbox.buffer);
    }
    
    // Set focus to textbox
    ui_set_focus(FOCUS_TEXTBOX, &g_ux_state.url_textbox);
    
    g_ux_state.ui_initialized = 1;
}

// Show ready screen with graphical UI (Step 9)
void ux_show_ready_screen(void) {
    graphics_clear_screen(COLOR_BLACK);
    
    // Title
    draw_text(2, 1, "AutismOS - Network Ping", COLOR_LIGHT_CYAN);
    draw_rect(1, 0, 78, 3, COLOR_LIGHT_GRAY);
    
    // IP address input label
    draw_text(2, 4, "IP Address:", COLOR_WHITE);
    
    // Render URL textbox
    textbox_render(&g_ux_state.url_textbox);
    
    // Instructions
    draw_text(2, 10, "Press ENTER to ping", COLOR_YELLOW);
    draw_text(2, 11, "Type to edit IP address", COLOR_LIGHT_GRAY);
    
    // Content area separator
    draw_rect(1, 13, 78, 11, COLOR_DARK_GRAY);
    draw_text(3, 14, "Ping results will appear here...", COLOR_LIGHT_GRAY);
}

// Finish boot sequence and transition to main UI
void ux_finish_boot(void) {
    g_ux_state.boot_complete = 1;
    g_ux_state.silent_mode = 1;
    
    // Initialize graphical UI (Step 9)
    ux_init_ui();
    
    // Show ready screen with textbox
    ux_show_ready_screen();
}

// Handle keyboard input for UI (Step 9)
void ux_handle_key_input(char ch) {
    if (!g_ux_state.ui_initialized) {
        return;
    }
    
    // Handle regular characters
    if (ch >= 32 && ch <= 126) {  // Printable ASCII
        textbox_handle_char(&g_ux_state.url_textbox, ch);
        ui_render_caret();
    }
    // Handle backspace
    else if (ch == '\b') {
        textbox_handle_backspace(&g_ux_state.url_textbox);
        ui_render_caret();
    }
}

// Handle Enter key - submit IP for ping (Step 9)
void ux_handle_enter_key(void) {
    if (!g_ux_state.ui_initialized) {
        return;
    }
    
    const char* ip = textbox_get_text(&g_ux_state.url_textbox);
    if (ip && ip[0]) {
        // Save the IP
        ux_save_ip(ip);
        
        // Show loading message in content area
        graphics_clear_region(2, 14, 76, 9, COLOR_BLACK);
        draw_text(3, 15, "Pinging: ", COLOR_WHITE);
        draw_text(13, 15, ip, COLOR_YELLOW);
        draw_text(3, 17, "Please wait...", COLOR_LIGHT_GRAY);
    }
}

// Get URL textbox (Step 9)
textbox_t* ux_get_url_textbox(void) {
    return &g_ux_state.url_textbox;
}

// Update caret blink (Step 9)
void ux_update_caret(void) {
    if (g_ux_state.ui_initialized) {
        ui_update_caret_blink();
        ui_render_caret();
    }
}

// Silent mode control
void ux_set_silent_mode(uint8 enabled) {
    g_ux_state.silent_mode = enabled;
}

uint8 ux_is_silent(void) {
    return g_ux_state.silent_mode;
}

// Persistence - IP address
void ux_save_ip(const char* ip) {
    if (ip) {
        strncpy(g_ux_state.last_ip, ip, sizeof(g_ux_state.last_ip) - 1);
        g_ux_state.last_ip[sizeof(g_ux_state.last_ip) - 1] = '\0';
    }
}

const char* ux_get_last_ip(void) {
    return g_ux_state.last_ip;
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
            // In Step 9, we maintain the graphical UI
            ux_set_active_process(1);
        }
    }
    // Alt+Q (0x51) - Quit
    else if (key == 'q' || key == 'Q') {
        if (g_ux_state.boot_complete) {
            graphics_clear_screen(COLOR_BLACK);
            draw_text(30, 12, "Quitting...", COLOR_YELLOW);
        }
    }
}

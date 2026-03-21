#ifndef THEME_H
#define THEME_H

#include "types.h"
#include "graphics.h"

/*
 * Theme System - AutismOS
 * 
 * Provides centralized color schemes and visual styling.
 * Makes it easy to change the look of the entire UI.
 */

// ============================================================================
// Theme Color Slots
// ============================================================================

typedef enum {
    // Background colors
    THEME_BG_PRIMARY = 0,      // Main background
    THEME_BG_SECONDARY,        // Secondary surfaces
    THEME_BG_TERTIARY,         // Tertiary surfaces (panels)
    THEME_BG_INPUT,            // Input fields background
    
    // Foreground colors
    THEME_FG_PRIMARY,          // Main text
    THEME_FG_SECONDARY,        // Secondary text (hints, labels)
    THEME_FG_DISABLED,         // Disabled text
    
    // Accent colors
    THEME_ACCENT_PRIMARY,      // Primary accent (buttons, links)
    THEME_ACCENT_SECONDARY,    // Secondary accent
    THEME_ACCENT_HOVER,        // Hover state
    
    // State colors
    THEME_STATE_NORMAL,
    THEME_STATE_HOVERED,
    THEME_STATE_PRESSED,
    THEME_STATE_FOCUSED,
    THEME_STATE_DISABLED,
    THEME_STATE_ERROR,
    THEME_STATE_SUCCESS,
    THEME_STATE_WARNING,
    
    // Border colors
    THEME_BORDER_NORMAL,
    THEME_BORDER_FOCUSED,
    THEME_BORDER_DISABLED,
    
    // Window colors
    THEME_WINDOW_TITLEBAR,
    THEME_WINDOW_TITLEBAR_FOCUSED,
    THEME_WINDOW_BORDER,
    THEME_WINDOW_CONTENT,
    
    // Count
    THEME_COLOR_COUNT
} theme_color_slot_t;

// ============================================================================
// Theme Structure
// ============================================================================

typedef struct {
    const char* name;
    uint8 colors[THEME_COLOR_COUNT];
    
    // Widget defaults
    uint8 button_height;
    uint8 textbox_height;
    uint8 checkbox_size;
    uint8 slider_height;
    uint8 progress_height;
    
    // Spacing
    uint8 padding_small;
    uint8 padding_medium;
    uint8 padding_large;
    uint8 spacing_small;
    uint8 spacing_medium;
    uint8 spacing_large;
    
    // Border widths
    uint8 border_thin;
    uint8 border_normal;
    uint8 border_thick;
    
    // Font
    uint8 font_scale;
    uint8 line_height;
} theme_t;

// ============================================================================
// Built-in Themes
// ============================================================================

extern const theme_t* g_theme_default;
extern const theme_t* g_theme_dark;
extern const theme_t* g_theme_light;
extern const theme_t* g_theme_hacker;

// ============================================================================
// Theme Functions
// ============================================================================

// Initialize theme system
void theme_init(void);

// Get current active theme
const theme_t* theme_get(void);

// Set active theme
void theme_set(const theme_t* theme);

// Get color from current theme
uint8 theme_color(theme_color_slot_t slot);

// Get color from specific theme
uint8 theme_get_color(const theme_t* theme, theme_color_slot_t slot);

// Get spacing values
uint8 theme_padding_small(void);
uint8 theme_padding_medium(void);
uint8 theme_padding_large(void);
uint8 theme_spacing_small(void);
uint8 theme_spacing_medium(void);
uint8 theme_spacing_large(void);

// Get widget defaults
uint8 theme_button_height(void);
uint8 theme_textbox_height(void);
uint8 theme_checkbox_size(void);
uint8 theme_slider_height(void);
uint8 theme_progress_height(void);

// ============================================================================
// Theme Helpers (for widgets)
// ============================================================================

// Get colors for a button in current state
void theme_get_button_colors(uint8 state, uint8* bg, uint8* fg, uint8* border);

// Get colors for a textbox in current state
void theme_get_textbox_colors(uint8 state, uint8* bg, uint8* fg, uint8* border);

// Get colors for a panel
void theme_get_panel_colors(uint8* bg, uint8* fg, uint8* border);

// Get window colors
void theme_get_window_colors(uint8 focused, uint8* titlebar, uint8* border, uint8* content);

#endif // THEME_H
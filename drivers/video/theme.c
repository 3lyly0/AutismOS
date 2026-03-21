#include "theme.h"
#include "graphics.h"

/*
 * Theme System Implementation - AutismOS
 */

// ============================================================================
// Built-in Theme Definitions
// ============================================================================

// Default theme (balanced, readable)
static const theme_t theme_default = {
    .name = "Default",
    .colors = {
        // Backgrounds
        [THEME_BG_PRIMARY]   = COLOR_LIGHT_GRAY,
        [THEME_BG_SECONDARY] = COLOR_DARK_GRAY,
        [THEME_BG_TERTIARY]  = COLOR_LIGHT_GRAY,
        [THEME_BG_INPUT]     = COLOR_BLACK,
        
        // Foregrounds
        [THEME_FG_PRIMARY]   = COLOR_BLACK,
        [THEME_FG_SECONDARY] = COLOR_DARK_GRAY,
        [THEME_FG_DISABLED]  = COLOR_DARK_GRAY,
        
        // Accents
        [THEME_ACCENT_PRIMARY]   = COLOR_BLUE,
        [THEME_ACCENT_SECONDARY] = COLOR_LIGHT_CYAN,
        [THEME_ACCENT_HOVER]     = COLOR_LIGHT_CYAN,
        
        // States
        [THEME_STATE_NORMAL]   = COLOR_LIGHT_GRAY,
        [THEME_STATE_HOVERED]  = COLOR_LIGHT_CYAN,
        [THEME_STATE_PRESSED]  = COLOR_DARK_GRAY,
        [THEME_STATE_FOCUSED]  = COLOR_LIGHT_CYAN,
        [THEME_STATE_DISABLED] = COLOR_DARK_GRAY,
        [THEME_STATE_ERROR]    = COLOR_LIGHT_RED,
        [THEME_STATE_SUCCESS]  = COLOR_LIGHT_GREEN,
        [THEME_STATE_WARNING]  = COLOR_YELLOW,
        
        // Borders
        [THEME_BORDER_NORMAL]  = COLOR_DARK_GRAY,
        [THEME_BORDER_FOCUSED] = COLOR_LIGHT_CYAN,
        [THEME_BORDER_DISABLED] = COLOR_DARK_GRAY,
        
        // Window
        [THEME_WINDOW_TITLEBAR]         = COLOR_DARK_GRAY,
        [THEME_WINDOW_TITLEBAR_FOCUSED] = COLOR_BLUE,
        [THEME_WINDOW_BORDER]           = COLOR_DARK_GRAY,
        [THEME_WINDOW_CONTENT]          = COLOR_LIGHT_GRAY,
    },
    
    // Widget defaults
    .button_height = 24,
    .textbox_height = 20,
    .checkbox_size = 14,
    .slider_height = 20,
    .progress_height = 16,
    
    // Spacing
    .padding_small  = 4,
    .padding_medium = 8,
    .padding_large  = 16,
    .spacing_small  = 4,
    .spacing_medium = 8,
    .spacing_large  = 16,
    
    // Borders
    .border_thin   = 1,
    .border_normal = 1,
    .border_thick  = 2,
    
    // Font
    .font_scale = 1,
    .line_height = 8
};

// Dark theme
static const theme_t theme_dark = {
    .name = "Dark",
    .colors = {
        // Backgrounds
        [THEME_BG_PRIMARY]   = COLOR_DARK_GRAY,
        [THEME_BG_SECONDARY] = COLOR_BLACK,
        [THEME_BG_TERTIARY]  = COLOR_DARK_GRAY,
        [THEME_BG_INPUT]     = COLOR_BLACK,
        
        // Foregrounds
        [THEME_FG_PRIMARY]   = COLOR_WHITE,
        [THEME_FG_SECONDARY] = COLOR_LIGHT_GRAY,
        [THEME_FG_DISABLED]  = COLOR_DARK_GRAY,
        
        // Accents
        [THEME_ACCENT_PRIMARY]   = COLOR_LIGHT_CYAN,
        [THEME_ACCENT_SECONDARY] = COLOR_CYAN,
        [THEME_ACCENT_HOVER]     = COLOR_CYAN,
        
        // States
        [THEME_STATE_NORMAL]   = COLOR_DARK_GRAY,
        [THEME_STATE_HOVERED]  = COLOR_CYAN,
        [THEME_STATE_PRESSED]  = COLOR_LIGHT_CYAN,
        [THEME_STATE_FOCUSED]  = COLOR_LIGHT_CYAN,
        [THEME_STATE_DISABLED] = COLOR_BLACK,
        [THEME_STATE_ERROR]    = COLOR_LIGHT_RED,
        [THEME_STATE_SUCCESS]  = COLOR_LIGHT_GREEN,
        [THEME_STATE_WARNING]  = COLOR_YELLOW,
        
        // Borders
        [THEME_BORDER_NORMAL]  = COLOR_LIGHT_GRAY,
        [THEME_BORDER_FOCUSED] = COLOR_LIGHT_CYAN,
        [THEME_BORDER_DISABLED] = COLOR_DARK_GRAY,
        
        // Window
        [THEME_WINDOW_TITLEBAR]         = COLOR_BLACK,
        [THEME_WINDOW_TITLEBAR_FOCUSED] = COLOR_DARK_GRAY,
        [THEME_WINDOW_BORDER]           = COLOR_LIGHT_GRAY,
        [THEME_WINDOW_CONTENT]          = COLOR_DARK_GRAY,
    },
    
    .button_height = 24,
    .textbox_height = 20,
    .checkbox_size = 14,
    .slider_height = 20,
    .progress_height = 16,
    
    .padding_small  = 4,
    .padding_medium = 8,
    .padding_large  = 16,
    .spacing_small  = 4,
    .spacing_medium = 8,
    .spacing_large  = 16,
    
    .border_thin   = 1,
    .border_normal = 1,
    .border_thick  = 2,
    
    .font_scale = 1,
    .line_height = 8
};

// Light theme
static const theme_t theme_light = {
    .name = "Light",
    .colors = {
        // Backgrounds
        [THEME_BG_PRIMARY]   = COLOR_WHITE,
        [THEME_BG_SECONDARY] = COLOR_LIGHT_GRAY,
        [THEME_BG_TERTIARY]  = COLOR_LIGHT_GRAY,
        [THEME_BG_INPUT]     = COLOR_WHITE,
        
        // Foregrounds
        [THEME_FG_PRIMARY]   = COLOR_BLACK,
        [THEME_FG_SECONDARY] = COLOR_DARK_GRAY,
        [THEME_FG_DISABLED]  = COLOR_LIGHT_GRAY,
        
        // Accents
        [THEME_ACCENT_PRIMARY]   = COLOR_BLUE,
        [THEME_ACCENT_SECONDARY] = COLOR_LIGHT_BLUE,
        [THEME_ACCENT_HOVER]     = COLOR_LIGHT_BLUE,
        
        // States
        [THEME_STATE_NORMAL]   = COLOR_WHITE,
        [THEME_STATE_HOVERED]  = COLOR_LIGHT_BLUE,
        [THEME_STATE_PRESSED]  = COLOR_LIGHT_GRAY,
        [THEME_STATE_FOCUSED]  = COLOR_BLUE,
        [THEME_STATE_DISABLED] = COLOR_LIGHT_GRAY,
        [THEME_STATE_ERROR]    = COLOR_RED,
        [THEME_STATE_SUCCESS]  = COLOR_GREEN,
        [THEME_STATE_WARNING]  = COLOR_BROWN,
        
        // Borders
        [THEME_BORDER_NORMAL]  = COLOR_DARK_GRAY,
        [THEME_BORDER_FOCUSED] = COLOR_BLUE,
        [THEME_BORDER_DISABLED] = COLOR_LIGHT_GRAY,
        
        // Window
        [THEME_WINDOW_TITLEBAR]         = COLOR_LIGHT_GRAY,
        [THEME_WINDOW_TITLEBAR_FOCUSED] = COLOR_BLUE,
        [THEME_WINDOW_BORDER]           = COLOR_DARK_GRAY,
        [THEME_WINDOW_CONTENT]          = COLOR_WHITE,
    },
    
    .button_height = 24,
    .textbox_height = 20,
    .checkbox_size = 14,
    .slider_height = 20,
    .progress_height = 16,
    
    .padding_small  = 4,
    .padding_medium = 8,
    .padding_large  = 16,
    .spacing_small  = 4,
    .spacing_medium = 8,
    .spacing_large  = 16,
    
    .border_thin   = 1,
    .border_normal = 1,
    .border_thick  = 2,
    
    .font_scale = 1,
    .line_height = 8
};

// Hacker theme (green on black, terminal style)
static const theme_t theme_hacker = {
    .name = "Hacker",
    .colors = {
        // Backgrounds
        [THEME_BG_PRIMARY]   = COLOR_BLACK,
        [THEME_BG_SECONDARY] = COLOR_BLACK,
        [THEME_BG_TERTIARY]  = COLOR_BLACK,
        [THEME_BG_INPUT]     = COLOR_BLACK,
        
        // Foregrounds
        [THEME_FG_PRIMARY]   = COLOR_LIGHT_GREEN,
        [THEME_FG_SECONDARY] = COLOR_GREEN,
        [THEME_FG_DISABLED]  = COLOR_DARK_GRAY,
        
        // Accents
        [THEME_ACCENT_PRIMARY]   = COLOR_GREEN,
        [THEME_ACCENT_SECONDARY] = COLOR_LIGHT_GREEN,
        [THEME_ACCENT_HOVER]     = COLOR_LIGHT_GREEN,
        
        // States
        [THEME_STATE_NORMAL]   = COLOR_BLACK,
        [THEME_STATE_HOVERED]  = COLOR_GREEN,
        [THEME_STATE_PRESSED]  = COLOR_LIGHT_GREEN,
        [THEME_STATE_FOCUSED]  = COLOR_LIGHT_GREEN,
        [THEME_STATE_DISABLED] = COLOR_DARK_GRAY,
        [THEME_STATE_ERROR]    = COLOR_LIGHT_RED,
        [THEME_STATE_SUCCESS]  = COLOR_LIGHT_GREEN,
        [THEME_STATE_WARNING]  = COLOR_YELLOW,
        
        // Borders
        [THEME_BORDER_NORMAL]  = COLOR_GREEN,
        [THEME_BORDER_FOCUSED] = COLOR_LIGHT_GREEN,
        [THEME_BORDER_DISABLED] = COLOR_DARK_GRAY,
        
        // Window
        [THEME_WINDOW_TITLEBAR]         = COLOR_BLACK,
        [THEME_WINDOW_TITLEBAR_FOCUSED] = COLOR_GREEN,
        [THEME_WINDOW_BORDER]           = COLOR_GREEN,
        [THEME_WINDOW_CONTENT]          = COLOR_BLACK,
    },
    
    .button_height = 24,
    .textbox_height = 20,
    .checkbox_size = 14,
    .slider_height = 20,
    .progress_height = 16,
    
    .padding_small  = 4,
    .padding_medium = 8,
    .padding_large  = 16,
    .spacing_small  = 4,
    .spacing_medium = 8,
    .spacing_large  = 16,
    
    .border_thin   = 1,
    .border_normal = 1,
    .border_thick  = 2,
    
    .font_scale = 1,
    .line_height = 8
};

// ============================================================================
// Theme Pointers (extern)
// ============================================================================

const theme_t* g_theme_default = &theme_default;
const theme_t* g_theme_dark    = &theme_dark;
const theme_t* g_theme_light   = &theme_light;
const theme_t* g_theme_hacker  = &theme_hacker;

// ============================================================================
// Current Theme State
// ============================================================================

static const theme_t* g_current_theme = &theme_default;

// ============================================================================
// Theme Functions
// ============================================================================

void theme_init(void) {
    g_current_theme = &theme_default;
}

const theme_t* theme_get(void) {
    return g_current_theme;
}

void theme_set(const theme_t* theme) {
    if (theme) {
        g_current_theme = theme;
    }
}

uint8 theme_color(theme_color_slot_t slot) {
    if (slot >= THEME_COLOR_COUNT) {
        return COLOR_WHITE;
    }
    return g_current_theme->colors[slot];
}

uint8 theme_get_color(const theme_t* theme, theme_color_slot_t slot) {
    if (!theme || slot >= THEME_COLOR_COUNT) {
        return COLOR_WHITE;
    }
    return theme->colors[slot];
}

// ============================================================================
// Spacing
// ============================================================================

uint8 theme_padding_small(void)  { return g_current_theme->padding_small; }
uint8 theme_padding_medium(void) { return g_current_theme->padding_medium; }
uint8 theme_padding_large(void)  { return g_current_theme->padding_large; }
uint8 theme_spacing_small(void)  { return g_current_theme->spacing_small; }
uint8 theme_spacing_medium(void) { return g_current_theme->spacing_medium; }
uint8 theme_spacing_large(void)  { return g_current_theme->spacing_large; }

// ============================================================================
// Widget Defaults
// ============================================================================

uint8 theme_button_height(void)  { return g_current_theme->button_height; }
uint8 theme_textbox_height(void) { return g_current_theme->textbox_height; }
uint8 theme_checkbox_size(void)  { return g_current_theme->checkbox_size; }
uint8 theme_slider_height(void)  { return g_current_theme->slider_height; }
uint8 theme_progress_height(void){ return g_current_theme->progress_height; }

// ============================================================================
// Theme Helpers
// ============================================================================

void theme_get_button_colors(uint8 state, uint8* bg, uint8* fg, uint8* border) {
    const theme_t* t = g_current_theme;
    
    switch (state) {
        case 1: // Hovered
            *bg = t->colors[THEME_STATE_HOVERED];
            *fg = t->colors[THEME_FG_PRIMARY];
            *border = t->colors[THEME_ACCENT_PRIMARY];
            break;
        case 2: // Pressed
            *bg = t->colors[THEME_STATE_PRESSED];
            *fg = t->colors[THEME_FG_PRIMARY];
            *border = t->colors[THEME_BORDER_NORMAL];
            break;
        case 3: // Disabled
            *bg = t->colors[THEME_STATE_DISABLED];
            *fg = t->colors[THEME_FG_DISABLED];
            *border = t->colors[THEME_BORDER_DISABLED];
            break;
        case 4: // Focused
            *bg = t->colors[THEME_STATE_NORMAL];
            *fg = t->colors[THEME_FG_PRIMARY];
            *border = t->colors[THEME_BORDER_FOCUSED];
            break;
        default: // Normal
            *bg = t->colors[THEME_STATE_NORMAL];
            *fg = t->colors[THEME_FG_PRIMARY];
            *border = t->colors[THEME_BORDER_NORMAL];
            break;
    }
}

void theme_get_textbox_colors(uint8 state, uint8* bg, uint8* fg, uint8* border) {
    const theme_t* t = g_current_theme;
    
    *bg = t->colors[THEME_BG_INPUT];
    *fg = t->colors[THEME_FG_PRIMARY];
    
    if (state == 3) { // Disabled
        *border = t->colors[THEME_BORDER_DISABLED];
    } else if (state == 4) { // Focused
        *border = t->colors[THEME_BORDER_FOCUSED];
    } else {
        *border = t->colors[THEME_BORDER_NORMAL];
    }
}

void theme_get_panel_colors(uint8* bg, uint8* fg, uint8* border) {
    const theme_t* t = g_current_theme;
    
    *bg = t->colors[THEME_BG_TERTIARY];
    *fg = t->colors[THEME_FG_PRIMARY];
    *border = t->colors[THEME_BORDER_NORMAL];
}

void theme_get_window_colors(uint8 focused, uint8* titlebar, uint8* border, uint8* content) {
    const theme_t* t = g_current_theme;
    
    *titlebar = focused ? 
        t->colors[THEME_WINDOW_TITLEBAR_FOCUSED] : 
        t->colors[THEME_WINDOW_TITLEBAR];
    *border = t->colors[THEME_WINDOW_BORDER];
    *content = t->colors[THEME_WINDOW_CONTENT];
}
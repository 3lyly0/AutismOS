#include "browser.h"
#include "desktop.h"
#include "graphics.h"
#include "string.h"
#include "tcp.h"
#include "ip.h"

static browser_state_t browser_states[MAX_WINDOWS];
static uint32 browser_count = 0;

static browser_state_t* get_browser_state(window_t* win) {
    if (browser_count < MAX_WINDOWS && win->app_data == NULL) {
        browser_state_t* state = &browser_states[browser_count++];
        memset(state, 0, sizeof(browser_state_t));
        strncpy(state->url, "10.0.2.2", sizeof(state->url) - 1);
        strncpy(state->status, "Enter IP and press Enter", sizeof(state->status) - 1);
        win->app_data = state;
        return state;
    }
    return (browser_state_t*)win->app_data;
}

void browser_draw(window_t* win) {
    if (!win) return;
    
    browser_state_t* state = get_browser_state(win);
    if (!state) return;
    
    volatile char* video = (volatile char*)0xB8000;
    
    // Content area
    uint32 content_y = win->y + 1;
    uint32 content_x = win->x + 1;
    uint32 content_width = win->width - 2;
    uint32 content_height = win->height - 2;
    
    // Clear content area
    uint8 bg_color = COLOR_BLACK;
    uint8 text_color = COLOR_WHITE;
    
    for (uint32 row = 0; row < content_height; row++) {
        for (uint32 col = 0; col < content_width; col++) {
            uint32 screen_row = content_y + row;
            uint32 screen_col = content_x + col;
            if (screen_row < 25 && screen_col < 80) {
                int index = (screen_row * 80 + screen_col) * 2;
                video[index] = ' ';
                video[index + 1] = (bg_color << 4) | text_color;
            }
        }
    }
    
    // Draw URL bar
    uint32 url_y = content_y;
    draw_text(content_x, url_y, "URL:", COLOR_YELLOW);
    
    // Draw URL text
    uint32 url_x = content_x + 5;
    for (uint32 i = 0; state->url[i] && url_x < content_x + content_width - 1; i++, url_x++) {
        int index = (url_y * 80 + url_x) * 2;
        video[index] = state->url[i];
        video[index + 1] = (COLOR_DARK_GRAY << 4) | COLOR_WHITE;
    }
    
    // Draw cursor at end of URL
    if (url_x < content_x + content_width - 1) {
        int index = (url_y * 80 + url_x) * 2;
        video[index] = '_';
        video[index + 1] = (COLOR_DARK_GRAY << 4) | COLOR_WHITE;
    }
    
    // Separator line
    for (uint32 col = content_x; col < content_x + content_width; col++) {
        int index = ((content_y + 1) * 80 + col) * 2;
        video[index] = '-';
        video[index + 1] = (bg_color << 4) | COLOR_DARK_GRAY;
    }
    
    // Status line
    draw_text(content_x, content_y + 2, state->status, 
              state->connected ? COLOR_LIGHT_GREEN : 
              state->loading ? COLOR_YELLOW : COLOR_LIGHT_GRAY);
    
    // Content area
    if (state->content[0]) {
        uint32 line_y = content_y + 4;
        uint32 col = 0;
        for (uint32 i = 0; state->content[i] && line_y < content_y + content_height; i++) {
            if (state->content[i] == '\n' || col >= content_width - 2) {
                line_y++;
                col = 0;
                if (state->content[i] == '\n') continue;
            }
            
            int index = (line_y * 80 + content_x + col) * 2;
            video[index] = state->content[i];
            video[index + 1] = (bg_color << 4) | COLOR_WHITE;
            col++;
        }
    }
}

static void browser_connect(browser_state_t* state) {
    if (!state || !state->url[0]) return;
    
    state->loading = 0;
    state->connected = 0;
    
    // Parse IP from URL
    uint32 ip = 0;
    uint8 octets[4] = {0};
    int octet_idx = 0;
    int val = 0;
    
    for (int i = 0; state->url[i] && octet_idx < 4; i++) {
        if (state->url[i] >= '0' && state->url[i] <= '9') {
            val = val * 10 + (state->url[i] - '0');
        } else if (state->url[i] == '.') {
            octets[octet_idx++] = val;
            val = 0;
        }
    }
    if (octet_idx < 4) {
        octets[octet_idx] = val;
    }
    
    ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    
    if (ip == 0) {
        strncpy(state->status, "Invalid IP address", sizeof(state->status) - 1);
        return;
    }
    
    // Show parsed IP for confirmation (simple manual formatting)
    state->status[0] = 'I';
    state->status[1] = 'P';
    state->status[2] = ':';
    state->status[3] = ' ';
    int pos = 4;
    for (int o = 0; o < 4; o++) {
        uint8 octet = octets[o];
        if (octet >= 100) {
            state->status[pos++] = '0' + (octet / 100);
            octet %= 100;
            state->status[pos++] = '0' + (octet / 10);
            state->status[pos++] = '0' + (octet % 10);
        } else if (octet >= 10) {
            state->status[pos++] = '0' + (octet / 10);
            state->status[pos++] = '0' + (octet % 10);
        } else {
            state->status[pos++] = '0' + octet;
        }
        if (o < 3) state->status[pos++] = '.';
    }
    state->status[pos] = '\0';
    
    // Simple ping test (ICMP) - non-blocking check
    strncpy(state->content, "Ready to connect.\n\nPress 'c' to test TCP\nPress 'p' to ping\n\nNote: QEMU only routes to:\n10.0.2.2 (gateway)\n10.0.2.3 (DNS)", sizeof(state->content) - 1);
    state->connected = 1;
}

void browser_handle_key(window_t* win, char key) {
    if (!win) return;
    
    browser_state_t* state = get_browser_state(win);
    if (!state) return;
    
    uint32 len = strlen(state->url);
    
    // Handle special commands when connected
    if (state->connected) {
        if (key == 'c' || key == 'C') {
            // Test TCP connection
            strncpy(state->status, "Testing TCP...", sizeof(state->status) - 1);
            browser_draw(win);
            
            // Parse IP again
            uint8 octets[4] = {0};
            int octet_idx = 0;
            int val = 0;
            for (int i = 0; state->url[i] && octet_idx < 4; i++) {
                if (state->url[i] >= '0' && state->url[i] <= '9') {
                    val = val * 10 + (state->url[i] - '0');
                } else if (state->url[i] == '.') {
                    octets[octet_idx++] = val;
                    val = 0;
                }
            }
            if (octet_idx < 4) octets[octet_idx] = val;
            uint32 ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
            
            int result = tcp_test_connection(ip);
            if (result > 0) {
                strncpy(state->status, "TCP: Connected!", sizeof(state->status) - 1);
                strncpy(state->content, "HTTP connection successful!\n\nServer responded on port 80.", sizeof(state->content) - 1);
            } else if (result == 0) {
                strncpy(state->status, "TCP: No response", sizeof(state->status) - 1);
                strncpy(state->content, "Connected but no HTTP response.\n\nServer may not have HTTP.", sizeof(state->content) - 1);
            } else {
                strncpy(state->status, "TCP: Failed", sizeof(state->status) - 1);
                strncpy(state->content, "Could not connect to server.\n\nCheck the IP address.", sizeof(state->content) - 1);
            }
            browser_draw(win);
            return;
        } else if (key == 'p' || key == 'P') {
            // Ping (ICMP)
            strncpy(state->status, "Pinging...", sizeof(state->status) - 1);
            strncpy(state->content, "ICMP ping in QEMU SLIRP\nonly works for:\n\n10.0.2.2 (gateway)\n10.0.2.3 (DNS)\n10.0.2.15 (this VM)", sizeof(state->content) - 1);
            browser_draw(win);
            return;
        } else if (key == 'r' || key == 'R') {
            // Reset - go back to URL editing
            state->connected = 0;
            strncpy(state->status, "Enter IP and press Enter", sizeof(state->status) - 1);
            memset(state->content, 0, sizeof(state->content));
            browser_draw(win);
            return;
        }
    }
    
    if (key == '\b') {  // Backspace
        if (len > 0) {
            state->url[len - 1] = '\0';
        }
    } else if (key == '\n') {  // Enter - connect
        browser_connect(state);
    } else if (key >= 32 && key <= 126) {  // Printable
        if (len < BROWSER_URL_MAX - 1) {
            state->url[len] = key;
            state->url[len + 1] = '\0';
        }
    }
    
    browser_draw(win);
}

window_t* browser_create(void) {
    window_t* win = desktop_create_window("Browser", 15, 3, 50, 18);
    if (!win) return NULL;
    
    win->draw_content = browser_draw;
    win->handle_key = browser_handle_key;
    win->app_data = NULL;
    
    // Initialize state
    get_browser_state(win);
    
    return win;
}

#ifndef BROWSER_H
#define BROWSER_H

#include "types.h"
#include "desktop.h"

#define BROWSER_URL_MAX 128

typedef struct {
    char url[BROWSER_URL_MAX];
    uint8 loading;
    uint8 connected;
    char status[64];
    char content[512];
} browser_state_t;

// Create browser window
window_t* browser_create(void);

// Browser callbacks
void browser_draw(window_t* win);
void browser_handle_key(window_t* win, char key);

#endif

#ifndef SYSINFO_H
#define SYSINFO_H

#include "types.h"
#include "desktop.h"

typedef struct {
    uint32 update_counter;
} sysinfo_state_t;

window_t* sysinfo_create(void);
void sysinfo_draw(window_t* win);
void sysinfo_handle_key(window_t* win, char key);

#endif

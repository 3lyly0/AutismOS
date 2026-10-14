#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define PIT_DEFAULT_HZ 1000

void timer_init(uint32 frequency_hz);
uint64 timer_get_ticks(void);
void sleep_ms(uint32 ms);

#endif

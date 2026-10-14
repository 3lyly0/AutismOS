#include "timer.h"
#include "io_ports.h"

extern volatile uint64 g_timer_ticks;

void timer_init(uint32 frequency_hz) {
    if (frequency_hz == 0) {
        frequency_hz = PIT_DEFAULT_HZ;
    }
    uint32 divisor = 1193180 / frequency_hz;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    // Command port 0x43: Channel 0, Access mode lobyte/hibyte, Mode 3 (square wave), 16-bit binary
    outportb(0x43, 0x36);

    // Channel 0 data port 0x40
    outportb(0x40, (uint8)(divisor & 0xFF));
    outportb(0x40, (uint8)((divisor >> 8) & 0xFF));
}

uint64 timer_get_ticks(void) {
    return g_timer_ticks;
}

void sleep_ms(uint32 ms) {
    uint32 eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    if (!(eflags & 0x200)) {
        for (volatile uint32 i = 0; i < ms * 10000; i++) {
            asm volatile("pause");
        }
        return;
    }
    uint64 start = g_timer_ticks;
    while ((g_timer_ticks - start) < (uint64)ms) {
        asm volatile("hlt");
    }
}

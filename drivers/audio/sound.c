#include "sound.h"
#include "io_ports.h"
#include "timer.h"

void play_sound(uint32_t frequency) {
    if (frequency == 0) return;
    uint32_t div = 1193180 / frequency;
    outportb(0x43, 0xB6);
    outportb(0x42, (uint8_t)(div & 0xFF));
    outportb(0x42, (uint8_t)((div >> 8) & 0xFF));

    uint8_t tmp = inportb(0x61);
    if (tmp != (tmp | 3)) {
        outportb(0x61, tmp | 3);
    }
}

void stop_sound(void) {
    uint8_t tmp = inportb(0x61) & 0xFC;
    outportb(0x61, tmp);
}

void beep(void) {
    play_sound(1000);
    sleep_ms(150);
    stop_sound();
}

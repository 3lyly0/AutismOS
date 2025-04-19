#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

void play_sound(uint32_t frequency);
void stop_sound();
void beep();

#endif
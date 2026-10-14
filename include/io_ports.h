#ifndef IO_PORTS_H
#define IO_PORTS_H

#include "types.h"

uint8 inportb(uint16 port);
void outportb(uint16 port, uint8 value);

uint16 inports(uint16 port);
void outports(uint16 port, uint16 data);

uint32 inportl(uint16 port);
void outportl(uint16 port, uint32 data);

static inline uint16 inportw(uint16 port) {
    return inports(port);
}

static inline void outportw(uint16 port, uint16 data) {
    outports(port, data);
}

#endif

#ifndef IO_PORTS_H
#define IO_PORTS_H
#include <stdint.h>
#include "types.h"


uint8 inportb(uint16 port);
void outportb(uint16_t port, uint8_t value);
uint16 inports(uint16 port);
void outports(uint16 port, uint16 data);
uint32 inportl(uint16 port);
void outportl(uint16 port, uint32 data);

#endif

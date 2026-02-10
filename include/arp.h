#ifndef ARP_H
#define ARP_H

#include "types.h"

#define ARP_CACHE_SIZE 8

void arp_init(void);
void arp_set_ip(uint32 ip);
void arp_request(uint32 ip);
int arp_lookup(uint32 ip, uint8* mac);
void arp_receive(uint8* packet, uint16 length);

#endif

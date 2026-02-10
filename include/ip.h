#ifndef IP_H
#define IP_H

#include "types.h"

#define IP_PROTO_ICMP 1

void ip_init(void);
void ip_set_config(uint32 ip, uint32 netmask, uint32 gateway);
void ip_send(uint32 dst_ip, uint8 protocol, const uint8* payload, uint16 length);
void ip_receive(uint8* packet, uint16 length);
uint32 ip_get_address(void);
void ip_set_icmp_callback(void (*callback)(uint8*, uint16, uint32));

#endif

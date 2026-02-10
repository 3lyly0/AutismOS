#ifndef ICMP_H
#define ICMP_H

#include "types.h"

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

typedef struct {
    uint8 received;
    uint32 src_ip;
} icmp_echo_state_t;

void icmp_init(void);
void icmp_send_echo_request(uint32 dst_ip, uint16 id, uint16 seq);
void icmp_receive(uint8* packet, uint16 length, uint32 src_ip);
int icmp_wait_reply(uint32* src_ip, uint32 timeout_ms);

#endif

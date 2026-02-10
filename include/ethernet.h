#ifndef ETHERNET_H
#define ETHERNET_H

#include "types.h"

#define ETH_ALEN 6
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

typedef struct __attribute__((packed)) {
    uint8 dst_mac[ETH_ALEN];
    uint8 src_mac[ETH_ALEN];
    uint16 type;
} eth_header_t;

void ethernet_init(void);
void ethernet_send(const uint8* dst_mac, uint16 type, const uint8* payload, uint16 length);
void ethernet_receive(uint8* packet, uint16 length);
const uint8* ethernet_get_mac(void);

#endif

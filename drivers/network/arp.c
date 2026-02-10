#include "arp.h"
#include "ethernet.h"
#include "string.h"

#define ARP_HTYPE_ETHERNET 0x0001
#define ARP_PTYPE_IPV4     0x0800
#define ARP_OP_REQUEST     0x0001
#define ARP_OP_REPLY       0x0002

typedef struct __attribute__((packed)) {
    uint16 htype;
    uint16 ptype;
    uint8 hlen;
    uint8 plen;
    uint16 op;
    uint8 sender_mac[6];
    uint32 sender_ip;
    uint8 target_mac[6];
    uint32 target_ip;
} arp_packet_t;

typedef struct {
    uint32 ip;
    uint8 mac[6];
    uint8 valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];
static uint32 local_ip = 0;

void arp_init(void) {
    memset(arp_cache, 0, sizeof(arp_cache));
}

void arp_set_ip(uint32 ip) {
    local_ip = ip;
}

void arp_request(uint32 ip) {
    arp_packet_t arp;
    
    arp.htype = (ARP_HTYPE_ETHERNET >> 8) | (ARP_HTYPE_ETHERNET << 8);
    arp.ptype = (ARP_PTYPE_IPV4 >> 8) | (ARP_PTYPE_IPV4 << 8);
    arp.hlen = 6;
    arp.plen = 4;
    arp.op = (ARP_OP_REQUEST >> 8) | (ARP_OP_REQUEST << 8);
    
    uint8* local_mac = ethernet_get_mac();
    memcpy(arp.sender_mac, local_mac, 6);
    arp.sender_ip = local_ip;
    
    memset(arp.target_mac, 0, 6);
    arp.target_ip = ip;
    
    uint8 broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    ethernet_send(broadcast, ETH_TYPE_ARP, (uint8*)&arp, sizeof(arp_packet_t));
}

int arp_lookup(uint32 ip, uint8* mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac, arp_cache[i].mac, 6);
            return 0;
        }
    }
    return -1;
}

void arp_receive(uint8* packet, uint16 length) {
    if (length < sizeof(arp_packet_t)) return;
    
    arp_packet_t* arp = (arp_packet_t*)packet;
    uint16 op = (arp->op >> 8) | (arp->op << 8);
    
    if (arp->target_ip == local_ip) {
        int found = 0;
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].valid && arp_cache[i].ip == arp->sender_ip) {
                found = 1;
                break;
            }
        }
        
        if (!found) {
            for (int i = 0; i < ARP_CACHE_SIZE; i++) {
                if (!arp_cache[i].valid) {
                    arp_cache[i].ip = arp->sender_ip;
                    memcpy(arp_cache[i].mac, arp->sender_mac, 6);
                    arp_cache[i].valid = 1;
                    break;
                }
            }
        }
        
        if (op == ARP_OP_REQUEST) {
            arp_packet_t reply;
            reply.htype = arp->htype;
            reply.ptype = arp->ptype;
            reply.hlen = 6;
            reply.plen = 4;
            reply.op = (ARP_OP_REPLY >> 8) | (ARP_OP_REPLY << 8);
            
            uint8* local_mac = ethernet_get_mac();
            memcpy(reply.sender_mac, local_mac, 6);
            reply.sender_ip = local_ip;
            
            memcpy(reply.target_mac, arp->sender_mac, 6);
            reply.target_ip = arp->sender_ip;
            
            ethernet_send(arp->sender_mac, ETH_TYPE_ARP, (uint8*)&reply, sizeof(arp_packet_t));
        }
    }
}

#include "ethernet.h"
#include "rtl8139.h"
#include "string.h"

static uint8 local_mac[ETH_ALEN];

static void (*arp_callback)(uint8*, uint16) = NULL;
static void (*ip_callback)(uint8*, uint16) = NULL;

void ethernet_init(void) {
    uint8* mac = rtl8139_get_mac_address();
    memcpy(local_mac, mac, ETH_ALEN);
}

void ethernet_set_arp_callback(void (*callback)(uint8*, uint16)) {
    arp_callback = callback;
}

void ethernet_set_ip_callback(void (*callback)(uint8*, uint16)) {
    ip_callback = callback;
}

void ethernet_send(const uint8* dst_mac, uint16 type, const uint8* payload, uint16 length) {
    uint8 frame[1518];
    eth_header_t* header = (eth_header_t*)frame;
    
    memcpy(header->dst_mac, dst_mac, ETH_ALEN);
    memcpy(header->src_mac, local_mac, ETH_ALEN);
    header->type = (type >> 8) | (type << 8);
    
    memcpy(frame + sizeof(eth_header_t), payload, length);
    
    rtl8139_send_packet(frame, sizeof(eth_header_t) + length);
}

void ethernet_receive(uint8* packet, uint16 length) {
    if (length < sizeof(eth_header_t)) return;
    
    eth_header_t* header = (eth_header_t*)packet;
    uint16 type = (header->type >> 8) | (header->type << 8);
    
    uint8* payload = packet + sizeof(eth_header_t);
    uint16 payload_len = length - sizeof(eth_header_t);
    
    if (type == ETH_TYPE_ARP && arp_callback) {
        arp_callback(payload, payload_len);
    } else if (type == ETH_TYPE_IP && ip_callback) {
        ip_callback(payload, payload_len);
    }
}

uint8* ethernet_get_mac(void) {
    return local_mac;
}

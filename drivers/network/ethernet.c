#include "ethernet.h"
#include "rtl8139.h"
#include "string.h"

static uint8 local_mac[ETH_ALEN];

static void (*arp_rx)(uint8*, uint16) = 0;
static void (*ip_rx)(uint8*, uint16)  = 0;

void ethernet_init(void) {
    const uint8* mac = rtl8139_get_mac_address();
    memcpy(local_mac, mac, ETH_ALEN);
}

void ethernet_set_arp_callback(void (*cb)(uint8*, uint16)) {
    arp_rx = cb;
}

void ethernet_set_ip_callback(void (*cb)(uint8*, uint16)) {
    ip_rx = cb;
}

void ethernet_send(const uint8* dst_mac,
                   uint16 eth_type,
                   const uint8* payload,
                   uint16 payload_len) {
    uint8 frame[1518];

    eth_header_t* hdr = (eth_header_t*)frame;

    memcpy(hdr->dst_mac, dst_mac, ETH_ALEN);
    memcpy(hdr->src_mac, local_mac, ETH_ALEN);
    hdr->type = (eth_type << 8) | (eth_type >> 8);

    memcpy(frame + sizeof(eth_header_t), payload, payload_len);

    rtl8139_send_packet(frame, sizeof(eth_header_t) + payload_len);
}

void ethernet_receive(uint8* frame, uint16 frame_len) {
    if (frame_len < sizeof(eth_header_t)) {
        return;
    }

    eth_header_t* hdr = (eth_header_t*)frame;
    uint16 eth_type = (hdr->type << 8) | (hdr->type >> 8);

    uint8* payload = frame + sizeof(eth_header_t);
    uint16 payload_len = frame_len - sizeof(eth_header_t);

    if (eth_type == ETH_TYPE_ARP && arp_rx) {
        arp_rx(payload, payload_len);
        return;
    }

    if (eth_type == ETH_TYPE_IP && ip_rx) {
        ip_rx(payload, payload_len);
        return;
    }
}

const uint8* ethernet_get_mac(void) {
    return local_mac;
}

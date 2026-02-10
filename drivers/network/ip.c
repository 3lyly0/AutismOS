#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "string.h"

#define IP_VERSION 4
#define IP_HEADER_LEN 5

typedef struct __attribute__((packed)) {
    uint8 version_ihl;
    uint8 tos;
    uint16 total_length;
    uint16 id;
    uint16 flags_offset;
    uint8 ttl;
    uint8 protocol;
    uint16 checksum;
    uint32 src_ip;
    uint32 dst_ip;
} ip_header_t;

static uint32 local_ip = 0;
static uint32 gateway_ip = 0;
static uint32 netmask = 0;
static uint16 ip_id = 0;

static void (*icmp_callback)(uint8*, uint16, uint32) = NULL;

void ip_init(void) {
}

void ip_set_config(uint32 ip, uint32 nm, uint32 gw) {
    local_ip = ip;
    netmask = nm;
    gateway_ip = gw;
}

void ip_set_icmp_callback(void (*callback)(uint8*, uint16, uint32)) {
    icmp_callback = callback;
}

static uint16 ip_checksum(uint16* data, int len) {
    uint32 sum = 0;
    while (len > 1) {
        sum += *data++;
        len -= 2;
    }
    if (len) {
        sum += *(uint8*)data;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void ip_send(uint32 dst_ip, uint8 protocol, const uint8* payload, uint16 length) {
    uint8 packet[1500];
    ip_header_t* header = (ip_header_t*)packet;
    
    header->version_ihl = (IP_VERSION << 4) | IP_HEADER_LEN;
    header->tos = 0;
    header->total_length = ((sizeof(ip_header_t) + length) >> 8) | ((sizeof(ip_header_t) + length) << 8);
    header->id = (ip_id >> 8) | (ip_id << 8);
    ip_id++;
    header->flags_offset = 0;
    header->ttl = 64;
    header->protocol = protocol;
    header->checksum = 0;
    header->src_ip = local_ip;
    header->dst_ip = dst_ip;
    
    header->checksum = ip_checksum((uint16*)header, sizeof(ip_header_t));
    
    memcpy(packet + sizeof(ip_header_t), payload, length);
    
    uint32 next_hop = ((dst_ip & netmask) == (local_ip & netmask)) ? dst_ip : gateway_ip;
    
    uint8 dst_mac[6];
    if (arp_lookup(next_hop, dst_mac) == 0) {
        ethernet_send(dst_mac, ETH_TYPE_IP, packet, sizeof(ip_header_t) + length);
    } else {
        arp_request(next_hop);
    }
}

void ip_receive(uint8* packet, uint16 length) {
    if (length < sizeof(ip_header_t)) return;
    
    ip_header_t* header = (ip_header_t*)packet;
    
    if (header->dst_ip != local_ip) return;
    
    uint8* payload = packet + sizeof(ip_header_t);
    uint16 total_len = (header->total_length >> 8) | (header->total_length << 8);
    uint16 payload_len = total_len - sizeof(ip_header_t);
    
    if (header->protocol == IP_PROTO_ICMP && icmp_callback) {
        icmp_callback(payload, payload_len, header->src_ip);
    }
}

uint32 ip_get_address(void) {
    return local_ip;
}

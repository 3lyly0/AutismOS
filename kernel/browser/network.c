#include "network.h"
#include "string.h"
#include "memory.h"
#include "pci.h"
#include "rtl8139.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "tcp.h"

extern void ethernet_set_arp_callback(void (*)(uint8*, uint16));
extern void ethernet_set_ip_callback(void (*)(uint8*, uint16));
extern void ip_set_icmp_callback(void (*)(uint8*, uint16, uint32));
extern void ip_set_tcp_callback(void (*)(uint8*, uint16, uint32));
extern void rtl8139_set_receive_callback(void (*)(uint8*, uint16));

void network_init(void) {
    pci_init();
    rtl8139_init();
    ethernet_init();
    arp_init();
    ip_init();
    icmp_init();
    tcp_init();
    
    rtl8139_set_receive_callback(ethernet_receive);
    ethernet_set_arp_callback(arp_receive);
    ethernet_set_ip_callback(ip_receive);
    ip_set_icmp_callback(icmp_receive);
    ip_set_tcp_callback(tcp_receive);
    
    uint32 local_ip = (10 << 0) | (0 << 8) | (2 << 16) | (15 << 24);
    uint32 netmask = (255 << 0) | (255 << 8) | (255 << 16) | (0 << 24);
    uint32 gateway = (10 << 0) | (0 << 8) | (2 << 16) | (2 << 24);
    
    ip_set_config(local_ip, netmask, gateway);
    arp_set_ip(local_ip);
}

int parse_url(const char* url_str, url_t* url) {
    if (!url_str || !url) {
        return -1;
    }
    
    memset(url, 0, sizeof(url_t));
    url->port = 80;
    
    const char* p = url_str;
    
    if (strncmp(p, "http://", 7) == 0) {
        strcpy(url->protocol, "http");
        p += 7;
    } else {
        strcpy(url->protocol, "http");
    }
    
    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    
    const char* end_of_authority = slash ? slash : (p + strlen(p));
    if (colon && colon < end_of_authority) {
        size_t host_len = colon - p;
        if (host_len >= sizeof(url->host)) {
            host_len = sizeof(url->host) - 1;
        }
        strncpy(url->host, p, host_len);
        url->host[host_len] = '\0';
        
        p = colon + 1;
        url->port = 0;
        while (*p >= '0' && *p <= '9' && *p != '/') {
            url->port = url->port * 10 + (*p - '0');
            p++;
        }
    } else {
        size_t host_len = end_of_authority - p;
        if (host_len >= sizeof(url->host)) {
            host_len = sizeof(url->host) - 1;
        }
        strncpy(url->host, p, host_len);
        url->host[host_len] = '\0';
    }
    
    if (slash) {
        strncpy(url->path, slash, sizeof(url->path) - 1);
        url->path[sizeof(url->path) - 1] = '\0';
    } else {
        strcpy(url->path, "/");
    }
    
    return 0;
}

int http_get(const char* host, const char* path, net_response_t* response) {
    (void)host;
    (void)path;
    (void)response;
    return -1;
}

static int is_valid_ip(const char* ip) {
    if (!ip) return 0;
    
    int dots = 0;
    int digits = 0;
    int value = 0;
    
    for (const char* p = ip; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            if (digits >= 3) return 0;
            
            int new_value = value * 10 + (*p - '0');
            if (new_value > 255) return 0;
            
            digits++;
            value = new_value;
        } else if (*p == '.') {
            if (digits == 0) return 0;
            dots++;
            digits = 0;
            value = 0;
        } else {
            return 0;
        }
    }
    
    return (dots == 3 && digits > 0);
}

static uint32 parse_ip_string(const char* ip_str) {
    uint32 ip = 0;
    uint8 octets[4] = {0};
    int octet_idx = 0;
    int value = 0;
    
    for (const char* p = ip_str; *p && octet_idx < 4; p++) {
        if (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
        } else if (*p == '.') {
            octets[octet_idx++] = value;
            value = 0;
        }
    }
    if (octet_idx == 3) {
        octets[3] = value;
    }
    
    ip = (octets[0] << 0) | (octets[1] << 8) | (octets[2] << 16) | (octets[3] << 24);
    return ip;
}

int ping_ip(const char* ip_address, ping_response_t* response) {
    if (!ip_address || !response) {
        return -1;
    }
    
    memset(response, 0, sizeof(ping_response_t));
    
    if (!is_valid_ip(ip_address)) {
        response->success = 0;
        strncpy(response->message, "Invalid IP address format", sizeof(response->message) - 1);
        return -1;
    }
    
    strncpy(response->ip_address, ip_address, sizeof(response->ip_address) - 1);
    
    uint32 dst_ip = parse_ip_string(ip_address);
    
    // First try ICMP ping
    icmp_send_echo_request(dst_ip, 1, 1);
    
    uint32 src_ip = 0;
    if (icmp_wait_reply(&src_ip, 1000) == 0) {
        response->success = 1;
        strncpy(response->message, "ICMP Ping OK - Host reachable", sizeof(response->message) - 1);
        return 0;
    }
    
    // ICMP failed, try TCP connection to port 80
    int tcp_result = tcp_test_connection(dst_ip);
    if (tcp_result == 1) {
        response->success = 1;
        strncpy(response->message, "TCP Connect OK - Got HTTP response", sizeof(response->message) - 1);
        return 0;
    } else if (tcp_result == 0) {
        response->success = 1;
        strncpy(response->message, "TCP Connect OK - No HTTP data", sizeof(response->message) - 1);
        return 0;
    }
    
    // Both failed
    response->success = 0;
    strncpy(response->message, "Connection failed - Host unreachable", sizeof(response->message) - 1);
    return -1;
}

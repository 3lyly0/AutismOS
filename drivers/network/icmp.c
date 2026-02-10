#include "icmp.h"
#include "ip.h"
#include "string.h"

typedef struct __attribute__((packed)) {
    uint8 type;
    uint8 code;
    uint16 checksum;
    uint16 id;
    uint16 sequence;
} icmp_header_t;

static icmp_echo_state_t echo_state;

void icmp_init(void) {
    memset(&echo_state, 0, sizeof(echo_state));
}

static uint16 icmp_checksum(uint16* data, int len) {
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

void icmp_send_echo_request(uint32 dst_ip, uint16 id, uint16 seq) {
    uint8 packet[64];
    icmp_header_t* header = (icmp_header_t*)packet;
    
    header->type = ICMP_TYPE_ECHO_REQUEST;
    header->code = 0;
    header->checksum = 0;
    header->id = (id >> 8) | (id << 8);
    header->sequence = (seq >> 8) | (seq << 8);
    
    for (int i = sizeof(icmp_header_t); i < 64; i++) {
        packet[i] = i;
    }
    
    header->checksum = icmp_checksum((uint16*)packet, 64);
    
    echo_state.received = 0;
    echo_state.src_ip = 0;
    
    ip_send(dst_ip, IP_PROTO_ICMP, packet, 64);
}

void icmp_receive(uint8* packet, uint16 length, uint32 src_ip) {
    if (length < sizeof(icmp_header_t)) return;
    
    icmp_header_t* header = (icmp_header_t*)packet;
    
    if (header->type == ICMP_TYPE_ECHO_REQUEST) {
        header->type = ICMP_TYPE_ECHO_REPLY;
        header->checksum = 0;
        header->checksum = icmp_checksum((uint16*)packet, length);
        
        ip_send(src_ip, IP_PROTO_ICMP, packet, length);
    } else if (header->type == ICMP_TYPE_ECHO_REPLY) {
        echo_state.received = 1;
        echo_state.src_ip = src_ip;
    }
}

#define DELAY_ITERATIONS_PER_MS 1000

int icmp_wait_reply(uint32* src_ip, uint32 timeout_ms) {
    extern void rtl8139_poll_receive(void);
    extern volatile uint64 g_timer_ticks;
    
    uint64 start_ticks = g_timer_ticks;
    uint64 timeout_ticks = timeout_ms / 10;  // Timer runs at ~100Hz
    
    if (timeout_ticks < 10) timeout_ticks = 10;  // Minimum 100ms
    
    while ((g_timer_ticks - start_ticks) < timeout_ticks) {
        // Poll for incoming packets
        rtl8139_poll_receive();
        
        // Check if we received a reply
        if (echo_state.received) {
            if (src_ip) {
                *src_ip = echo_state.src_ip;
            }
            return 0;  // Success
        }
        
        // Small delay and yield to other processes
        for (volatile int j = 0; j < DELAY_ITERATIONS_PER_MS; j++);
        
        // Allow interrupts and yield
        asm volatile("sti");
        asm volatile("hlt");  // Wait for next interrupt
    }
    
    return -1;  // Timeout
}

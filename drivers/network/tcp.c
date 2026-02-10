#include "tcp.h"
#include "ip.h"
#include "string.h"

typedef struct __attribute__((packed)) {
    uint16 src_port;
    uint16 dst_port;
    uint32 seq_num;
    uint32 ack_num;
    uint8 data_offset;  // Upper 4 bits = header length in 32-bit words
    uint8 flags;
    uint16 window;
    uint16 checksum;
    uint16 urgent_ptr;
} tcp_header_t;

// Pseudo header for checksum calculation
typedef struct __attribute__((packed)) {
    uint32 src_ip;
    uint32 dst_ip;
    uint8 zero;
    uint8 protocol;
    uint16 tcp_length;
} pseudo_header_t;

#define IP_PROTO_TCP 6

static tcp_connection_t* active_conn = NULL;
static uint16 next_local_port = 49152;

// External functions
extern void rtl8139_poll_receive(void);
extern volatile uint64 g_timer_ticks;
extern uint32 ip_get_local(void);

void tcp_init(void) {
    active_conn = NULL;
    next_local_port = 49152;
}

static uint16 htons(uint16 val) {
    return (val >> 8) | (val << 8);
}

static uint32 htonl(uint32 val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

static uint16 tcp_checksum(uint32 src_ip, uint32 dst_ip, uint8* tcp_segment, uint16 tcp_len) {
    uint32 sum = 0;
    
    // Pseudo header
    pseudo_header_t pseudo;
    pseudo.src_ip = src_ip;
    pseudo.dst_ip = dst_ip;
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTO_TCP;
    pseudo.tcp_length = htons(tcp_len);
    
    uint16* ptr = (uint16*)&pseudo;
    for (int i = 0; i < 6; i++) {
        sum += ptr[i];
    }
    
    // TCP segment
    ptr = (uint16*)tcp_segment;
    int words = tcp_len / 2;
    for (int i = 0; i < words; i++) {
        sum += ptr[i];
    }
    
    // Odd byte
    if (tcp_len & 1) {
        sum += tcp_segment[tcp_len - 1];
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

static void tcp_send_packet(tcp_connection_t* conn, uint8 flags, const uint8* data, uint16 data_len) {
    uint8 packet[1500];
    tcp_header_t* header = (tcp_header_t*)packet;
    
    memset(packet, 0, sizeof(tcp_header_t));
    
    header->src_port = htons(conn->local_port);
    header->dst_port = htons(conn->remote_port);
    header->seq_num = htonl(conn->seq_num);
    header->ack_num = htonl(conn->ack_num);
    header->data_offset = (5 << 4);  // 5 * 4 = 20 bytes header
    header->flags = flags;
    header->window = htons(8192);
    header->checksum = 0;
    header->urgent_ptr = 0;
    
    if (data && data_len > 0) {
        memcpy(packet + sizeof(tcp_header_t), data, data_len);
    }
    
    uint16 total_len = sizeof(tcp_header_t) + data_len;
    
    // Calculate checksum
    extern uint32 ip_get_local(void);
    uint32 local_ip = ip_get_local();
    header->checksum = tcp_checksum(local_ip, conn->remote_ip, packet, total_len);
    
    ip_send(conn->remote_ip, IP_PROTO_TCP, packet, total_len);
}

int tcp_connect(uint32 ip, uint16 port, tcp_connection_t* conn) {
    if (!conn) return -1;
    
    memset(conn, 0, sizeof(tcp_connection_t));
    
    conn->remote_ip = ip;
    conn->remote_port = port;
    conn->local_port = next_local_port++;
    conn->seq_num = g_timer_ticks * 12345;  // Simple pseudo-random
    conn->ack_num = 0;
    conn->state = TCP_SYN_SENT;
    conn->recv_len = 0;
    conn->data_ready = 0;
    
    active_conn = conn;
    
    // Send SYN
    tcp_send_packet(conn, TCP_FLAG_SYN, NULL, 0);
    conn->seq_num++;
    
    // Wait for SYN-ACK with timeout
    uint64 start = g_timer_ticks;
    uint64 timeout = 300;  // 3 seconds at 100Hz
    
    while ((g_timer_ticks - start) < timeout) {
        rtl8139_poll_receive();
        
        if (conn->state == TCP_ESTABLISHED) {
            return 0;  // Success!
        }
        
        // Small delay
        for (volatile int i = 0; i < 1000; i++);
    }
    
    // Timeout - connection failed
    conn->state = TCP_CLOSED;
    active_conn = NULL;
    return -1;
}

int tcp_send(tcp_connection_t* conn, const uint8* data, uint16 len) {
    if (!conn || conn->state != TCP_ESTABLISHED) return -1;
    
    tcp_send_packet(conn, TCP_FLAG_PSH | TCP_FLAG_ACK, data, len);
    conn->seq_num += len;
    
    return len;
}

int tcp_recv(tcp_connection_t* conn, uint8* buffer, uint16 max_len) {
    if (!conn) return -1;
    
    // Poll for incoming packets
    rtl8139_poll_receive();
    
    if (conn->data_ready && conn->recv_len > 0) {
        uint16 copy_len = (conn->recv_len < max_len) ? conn->recv_len : max_len;
        memcpy(buffer, conn->recv_buffer, copy_len);
        conn->recv_len = 0;
        conn->data_ready = 0;
        return copy_len;
    }
    
    return 0;
}

void tcp_close(tcp_connection_t* conn) {
    if (!conn || conn->state == TCP_CLOSED) return;
    
    // Send FIN
    tcp_send_packet(conn, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL, 0);
    conn->state = TCP_CLOSED;
    
    if (active_conn == conn) {
        active_conn = NULL;
    }
}

void tcp_receive(uint8* packet, uint16 length, uint32 src_ip) {
    if (length < sizeof(tcp_header_t)) return;
    if (!active_conn) return;
    
    tcp_header_t* header = (tcp_header_t*)packet;
    
    uint16 src_port = htons(header->src_port);
    uint16 dst_port = htons(header->dst_port);
    uint32 seq = htonl(header->seq_num);
    uint32 ack = htonl(header->ack_num);
    
    // Check if this packet is for our connection
    if (src_ip != active_conn->remote_ip ||
        src_port != active_conn->remote_port ||
        dst_port != active_conn->local_port) {
        return;
    }
    
    uint8 flags = header->flags;
    uint8 header_len = (header->data_offset >> 4) * 4;
    uint16 data_len = length - header_len;
    uint8* data = packet + header_len;
    
    switch (active_conn->state) {
        case TCP_SYN_SENT:
            // Expecting SYN-ACK
            if ((flags & TCP_FLAG_SYN) && (flags & TCP_FLAG_ACK)) {
                active_conn->ack_num = seq + 1;
                active_conn->state = TCP_ESTABLISHED;
                
                // Send ACK
                tcp_send_packet(active_conn, TCP_FLAG_ACK, NULL, 0);
            }
            break;
            
        case TCP_ESTABLISHED:
            // Received data or ACK
            if (data_len > 0) {
                // Store received data
                uint16 copy_len = data_len;
                if (copy_len > sizeof(active_conn->recv_buffer)) {
                    copy_len = sizeof(active_conn->recv_buffer);
                }
                memcpy(active_conn->recv_buffer, data, copy_len);
                active_conn->recv_len = copy_len;
                active_conn->data_ready = 1;
                active_conn->ack_num = seq + data_len;
                
                // Send ACK
                tcp_send_packet(active_conn, TCP_FLAG_ACK, NULL, 0);
            }
            
            // Check for FIN
            if (flags & TCP_FLAG_FIN) {
                active_conn->ack_num = seq + 1;
                tcp_send_packet(active_conn, TCP_FLAG_ACK, NULL, 0);
                active_conn->state = TCP_CLOSED;
            }
            break;
            
        default:
            break;
    }
}

// Simple connection test
int tcp_test_connection(uint32 ip) {
    tcp_connection_t conn;
    
    if (tcp_connect(ip, 80, &conn) == 0) {
        // Connected! Send simple HTTP request
        const char* http_req = "GET / HTTP/1.0\r\nHost: test\r\n\r\n";
        tcp_send(&conn, (const uint8*)http_req, strlen(http_req));
        
        // Wait for response
        uint8 buffer[512];
        uint64 start = g_timer_ticks;
        
        while ((g_timer_ticks - start) < 300) {  // 3 sec timeout
            int received = tcp_recv(&conn, buffer, sizeof(buffer) - 1);
            if (received > 0) {
                tcp_close(&conn);
                return 1;  // Success - got HTTP response!
            }
            for (volatile int i = 0; i < 1000; i++);
        }
        
        tcp_close(&conn);
        return 0;  // Connected but no response
    }
    
    return -1;  // Connection failed
}

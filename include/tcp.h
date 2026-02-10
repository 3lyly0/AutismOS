#ifndef TCP_H
#define TCP_H

#include "types.h"

#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_URG  0x20

// TCP connection states
typedef enum {
    TCP_CLOSED,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT,
    TCP_CLOSE_WAIT
} tcp_state_t;

// TCP connection structure
typedef struct {
    uint32 remote_ip;
    uint16 local_port;
    uint16 remote_port;
    uint32 seq_num;
    uint32 ack_num;
    tcp_state_t state;
    uint8 recv_buffer[2048];
    uint16 recv_len;
    uint8 data_ready;
} tcp_connection_t;

// Initialize TCP subsystem
void tcp_init(void);

// Open a TCP connection (blocking)
// Returns 0 on success, -1 on failure
int tcp_connect(uint32 ip, uint16 port, tcp_connection_t* conn);

// Send data over TCP connection
int tcp_send(tcp_connection_t* conn, const uint8* data, uint16 len);

// Receive data from TCP connection (non-blocking)
// Returns number of bytes received, 0 if no data, -1 on error
int tcp_recv(tcp_connection_t* conn, uint8* buffer, uint16 max_len);

// Close TCP connection
void tcp_close(tcp_connection_t* conn);

// Called by IP layer when TCP packet received
void tcp_receive(uint8* packet, uint16 length, uint32 src_ip);

// Simple connection test - returns 1 if can reach server on port 80
int tcp_test_connection(uint32 ip);

#endif

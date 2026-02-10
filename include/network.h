#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

// Network request types
#define NET_REQUEST_HTTP_GET  1

// Network response structure
typedef struct net_response {
    uint32 status_code;      // HTTP status code (200, 404, etc.)
    uint32 content_length;   // Length of content
    char* content;           // Response content (allocated)
} net_response_t;

// URL structure
typedef struct url {
    char protocol[16];       // "http" or "https"
    char host[256];          // hostname
    uint16 port;             // port number (default 80)
    char path[256];          // path (default "/")
} url_t;

// Ping response structure
typedef struct ping_response {
    uint8 success;           // 1 if ping succeeded, 0 if failed
    char ip_address[16];     // IP address that was pinged
    char message[256];       // Status message
} ping_response_t;

// Network functions
void network_init(void);
int parse_url(const char* url_str, url_t* url);
int http_get(const char* host, const char* path, net_response_t* response);
int ping_ip(const char* ip_address, ping_response_t* response);

#endif

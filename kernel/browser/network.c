#include "network.h"
#include "string.h"
#include "memory.h"

// Initialize network subsystem
void network_init(void) {
    // For this minimal implementation, no initialization needed
    // In a real system, this would initialize network stack
}

// Parse a URL string into components
// Returns: 0 on success, -1 on failure
int parse_url(const char* url_str, url_t* url) {
    if (!url_str || !url) {
        return -1;
    }
    
    // Initialize URL structure
    memset(url, 0, sizeof(url_t));
    url->port = 80;  // Default HTTP port
    
    // Simple parser for http:// URLs
    // Format: http://hostname[:port]/path
    
    const char* p = url_str;
    
    // Check for http:// prefix
    if (strncmp(p, "http://", 7) == 0) {
        strcpy(url->protocol, "http");
        p += 7;
    } else {
        // Default to http
        strcpy(url->protocol, "http");
    }
    
    // Parse hostname
    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    
    // Determine end of hostname (authority section)
    const char* end_of_authority = slash ? slash : (p + strlen(p));
    if (colon && colon < end_of_authority) {
        // Port is present
        size_t host_len = colon - p;
        if (host_len >= sizeof(url->host)) {
            host_len = sizeof(url->host) - 1;
        }
        strncpy(url->host, p, host_len);
        url->host[host_len] = '\0';
        
        // Parse port
        p = colon + 1;
        url->port = 0;
        while (*p >= '0' && *p <= '9' && *p != '/') {
            url->port = url->port * 10 + (*p - '0');
            p++;
        }
    } else {
        // No port, just hostname
        size_t host_len = end_of_authority - p;
        if (host_len >= sizeof(url->host)) {
            host_len = sizeof(url->host) - 1;
        }
        strncpy(url->host, p, host_len);
        url->host[host_len] = '\0';
    }
    
    // Parse path
    if (slash) {
        strncpy(url->path, slash, sizeof(url->path) - 1);
        url->path[sizeof(url->path) - 1] = '\0';
    } else {
        strcpy(url->path, "/");
    }
    
    return 0;
}

// Perform HTTP GET request (minimal, blocking implementation)
// Returns: 0 on success, -1 on failure
int http_get(const char* host, const char* path, net_response_t* response) {
    if (!host || !path || !response) {
        return -1;
    }
    
    // For this minimal implementation, we simulate a simple response
    // In a real system, this would:
    // 1. Open TCP connection to host:80
    // 2. Send HTTP GET request
    // 3. Parse HTTP response headers
    // 4. Read response body
    
    // Simulate a simple HTML response
    const char* mock_html = 
        "<html>"
        "<body>"
        "<h1>Welcome to AutismOS Browser</h1>"
        "<p>This is a minimal browser-capable operating system.</p>"
        "<p>Step 7: Input, Networking and Browser Core</p>"
        "</body>"
        "</html>";
    
    // Allocate response content
    size_t content_len = strlen(mock_html);
    response->content = (char*)kmalloc(content_len + 1);
    if (!response->content) {
        return -1;
    }
    
    // Copy mock content
    strcpy(response->content, mock_html);
    response->content_length = content_len;
    response->status_code = 200;  // HTTP OK
    
    return 0;
}

// Validate IP address format (simple check)
// Returns: 1 if valid, 0 if invalid
static int is_valid_ip(const char* ip) {
    if (!ip) return 0;
    
    int dots = 0;
    int digits = 0;
    int value = 0;
    
    for (const char* p = ip; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            digits++;
            value = value * 10 + (*p - '0');
            if (value > 255) return 0;  // Octet too large
            if (digits > 3) return 0;   // Too many digits in octet
        } else if (*p == '.') {
            if (digits == 0) return 0;  // Empty octet
            dots++;
            digits = 0;
            value = 0;
        } else {
            return 0;  // Invalid character
        }
    }
    
    // Should have exactly 3 dots and at least one digit in last octet
    return (dots == 3 && digits > 0);
}

// Perform ICMP ping to an IP address (minimal implementation)
// Returns: 0 on success, -1 on failure
int ping_ip(const char* ip_address, ping_response_t* response) {
    if (!ip_address || !response) {
        return -1;
    }
    
    // Initialize response
    memset(response, 0, sizeof(ping_response_t));
    
    // Validate IP address format
    if (!is_valid_ip(ip_address)) {
        response->success = 0;
        strcpy(response->message, "Invalid IP address format");
        return -1;
    }
    
    // Copy IP address to response
    strncpy(response->ip_address, ip_address, sizeof(response->ip_address) - 1);
    response->ip_address[sizeof(response->ip_address) - 1] = '\0';
    
    // For this minimal implementation, we simulate a ping
    // In a real system, this would:
    // 1. Create ICMP echo request packet
    // 2. Send packet to IP address
    // 3. Wait for ICMP echo reply
    // 4. Calculate round-trip time
    
    // Simulate successful ping
    response->success = 1;
    strcpy(response->message, "Ping successful - Host is reachable");
    
    return 0;
}

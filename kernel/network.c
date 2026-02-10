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
    
    // Determine end of hostname
    const char* host_end = slash ? slash : (p + strlen(p));
    if (colon && colon < host_end) {
        host_end = colon;
    }
    
    // Copy hostname
    size_t host_len = host_end - p;
    if (host_len >= sizeof(url->host)) {
        host_len = sizeof(url->host) - 1;
    }
    strncpy(url->host, p, host_len);
    url->host[host_len] = '\0';
    
    // Parse port if present
    if (colon && colon < (slash ? slash : (p + strlen(p)))) {
        url->port = 0;
        p = colon + 1;
        while (*p >= '0' && *p <= '9' && *p != '/') {
            url->port = url->port * 10 + (*p - '0');
            p++;
        }
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

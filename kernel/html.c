#include "html.h"
#include "string.h"
#include "memory.h"

// Initialize HTML parser
void html_init(void) {
    // No initialization needed for minimal parser
}

// Convert tag string to tag ID
uint32 html_tag_from_string(const char* tag_name) {
    if (strcmp(tag_name, "html") == 0) return TAG_HTML;
    if (strcmp(tag_name, "body") == 0) return TAG_BODY;
    if (strcmp(tag_name, "p") == 0) return TAG_P;
    if (strcmp(tag_name, "h1") == 0) return TAG_H1;
    if (strcmp(tag_name, "a") == 0) return TAG_A;
    return TAG_UNKNOWN;
}

// Allocate a new HTML node
static html_node_t* html_node_create(uint32 type) {
    html_node_t* node = (html_node_t*)kmalloc(sizeof(html_node_t));
    if (!node) {
        return NULL;
    }
    
    memset(node, 0, sizeof(html_node_t));
    node->type = type;
    return node;
}

// Simple HTML parser (extremely minimal)
// Only parses: <html>, <body>, <h1>, <p>, <a>
// Does not handle attributes or nested structures properly
html_node_t* html_parse(const char* html_string) {
    if (!html_string) {
        return NULL;
    }
    
    // Create root node
    html_node_t* root = html_node_create(NODE_TYPE_ELEMENT);
    if (!root) {
        return NULL;
    }
    root->tag = TAG_HTML;
    
    // This is a very simplified parser
    // In a real implementation, this would be a proper HTML parser
    // For now, we just extract text from <h1> and <p> tags
    
    const char* p = html_string;
    html_node_t* current_parent = root;
    html_node_t* last_child = NULL;
    
    while (*p) {
        // Look for opening tag
        if (*p == '<' && *(p + 1) != '/') {
            p++; // Skip '<'
            
            // Extract tag name
            char tag_name[32];
            int i = 0;
            while (*p && *p != '>' && *p != ' ' && i < 31) {
                tag_name[i++] = *p++;
            }
            tag_name[i] = '\0';
            
            // Skip to end of opening tag
            while (*p && *p != '>') p++;
            if (*p == '>') p++;
            
            // Create element node
            html_node_t* elem = html_node_create(NODE_TYPE_ELEMENT);
            if (elem) {
                elem->tag = html_tag_from_string(tag_name);
                
                // Add to tree
                if (!root->first_child) {
                    root->first_child = elem;
                } else if (last_child) {
                    last_child->next_sibling = elem;
                }
                last_child = elem;
                
                // Extract text content for h1 and p tags
                if (elem->tag == TAG_H1 || elem->tag == TAG_P) {
                    char text[256];
                    int text_len = 0;
                    while (*p && *p != '<' && text_len < 255) {
                        text[text_len++] = *p++;
                    }
                    text[text_len] = '\0';
                    strncpy(elem->text, text, sizeof(elem->text) - 1);
                }
            }
        } else {
            p++;
        }
    }
    
    return root;
}

// Free HTML tree
void html_free(html_node_t* node) {
    if (!node) {
        return;
    }
    
    // Free children recursively
    html_node_t* child = node->first_child;
    while (child) {
        html_node_t* next = child->next_sibling;
        html_free(child);
        child = next;
    }
    
    // Free this node
    kfree(node);
}

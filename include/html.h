#ifndef HTML_H
#define HTML_H

#include "types.h"

// HTML node types
#define NODE_TYPE_ELEMENT  1
#define NODE_TYPE_TEXT     2

// HTML tag types
#define TAG_HTML    1
#define TAG_BODY    2
#define TAG_P       3
#define TAG_H1      4
#define TAG_A       5
#define TAG_UNKNOWN 0

// HTML node structure
typedef struct html_node {
    uint32 type;                    // NODE_TYPE_ELEMENT or NODE_TYPE_TEXT
    uint32 tag;                     // Tag type (for elements)
    char text[256];                 // Text content
    struct html_node* first_child;  // First child node
    struct html_node* next_sibling; // Next sibling node
} html_node_t;

// HTML parser functions
void html_init(void);
html_node_t* html_parse(const char* html_string);
void html_free(html_node_t* node);
uint32 html_tag_from_string(const char* tag_name);

#endif

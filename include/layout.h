#ifndef LAYOUT_H
#define LAYOUT_H

#include "types.h"
#include "html.h"
#include "shm.h"

// Layout box structure (simple vertical flow)
typedef struct layout_box {
    uint32 x;
    uint32 y;
    uint32 width;
    uint32 height;
    html_node_t* node;           // Associated HTML node
    struct layout_box* next;      // Next box in layout
} layout_box_t;

// Layout tree (collection of boxes)
typedef struct layout_tree {
    layout_box_t* first_box;
    uint32 total_height;
} layout_tree_t;

// Layout functions
void layout_init(void);
layout_tree_t* layout_create_tree(html_node_t* html_root, uint32 viewport_width);
void layout_free_tree(layout_tree_t* tree);
void layout_render_to_framebuffer(layout_tree_t* tree, framebuffer_t* fb);

#endif

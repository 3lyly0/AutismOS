#include "layout.h"
#include "memory.h"
#include "string.h"

// Initialize layout engine
void layout_init(void) {
    // No initialization needed
}

// Create a layout box
static layout_box_t* layout_box_create(html_node_t* node, uint32 x, uint32 y, uint32 width) {
    layout_box_t* box = (layout_box_t*)kmalloc(sizeof(layout_box_t));
    if (!box) {
        return NULL;
    }
    
    memset(box, 0, sizeof(layout_box_t));
    box->node = node;
    box->x = x;
    box->y = y;
    box->width = width;
    
    // Calculate height based on content
    // For now, fixed heights based on tag type
    if (node->tag == TAG_H1) {
        box->height = 20;  // Larger for headings
    } else if (node->tag == TAG_P) {
        box->height = 12;  // Smaller for paragraphs
    } else {
        box->height = 10;
    }
    
    return box;
}

// Create layout tree from HTML tree (simple vertical flow)
layout_tree_t* layout_create_tree(html_node_t* html_root, uint32 viewport_width) {
    if (!html_root) {
        return NULL;
    }
    
    layout_tree_t* tree = (layout_tree_t*)kmalloc(sizeof(layout_tree_t));
    if (!tree) {
        return NULL;
    }
    
    memset(tree, 0, sizeof(layout_tree_t));
    
    // Simple vertical flow layout
    uint32 current_y = 5;  // Start with some padding
    layout_box_t* last_box = NULL;
    
    // Traverse HTML tree and create boxes
    html_node_t* node = html_root->first_child;
    while (node) {
        if (node->type == NODE_TYPE_ELEMENT && 
            (node->tag == TAG_H1 || node->tag == TAG_P)) {
            
            // Create layout box for this element
            layout_box_t* box = layout_box_create(node, 5, current_y, viewport_width - 10);
            if (box) {
                // Add to layout tree
                if (!tree->first_box) {
                    tree->first_box = box;
                } else if (last_box) {
                    last_box->next = box;
                }
                last_box = box;
                
                // Move to next vertical position
                current_y += box->height + 5;  // Add spacing
            }
        }
        
        node = node->next_sibling;
    }
    
    tree->total_height = current_y;
    return tree;
}

// Free layout tree
void layout_free_tree(layout_tree_t* tree) {
    if (!tree) {
        return;
    }
    
    // Free all boxes
    layout_box_t* box = tree->first_box;
    while (box) {
        layout_box_t* next = box->next;
        kfree(box);
        box = next;
    }
    
    kfree(tree);
}

// Simple character rendering (fixed-width font simulation)
static void render_char(framebuffer_t* fb, char c, uint32 x, uint32 y, uint32 color) {
    // Very simple 8x8 character rendering (just a placeholder)
    // In a real system, this would use actual font data
    if (!fb || !fb->pixels) return;
    
    // Draw a simple filled rectangle as a character placeholder
    for (uint32 dy = 0; dy < 8; dy++) {
        for (uint32 dx = 0; dx < 6; dx++) {
            uint32 px = x + dx;
            uint32 py = y + dy;
            if (px < fb->width && py < fb->height) {
                fb->pixels[py * fb->width + px] = color;
            }
        }
    }
}

// Render layout tree to framebuffer
void layout_render_to_framebuffer(layout_tree_t* tree, framebuffer_t* fb) {
    if (!tree || !fb || !fb->pixels) {
        return;
    }
    
    // Clear framebuffer to white/light gray
    for (uint32 i = 0; i < fb->width * fb->height; i++) {
        fb->pixels[i] = 0xF0F0F0;  // Light gray background
    }
    
    // Render each box
    layout_box_t* box = tree->first_box;
    while (box) {
        if (box->node && box->node->text[0]) {
            // Determine color based on tag
            uint32 text_color = 0x000000;  // Black
            if (box->node->tag == TAG_H1) {
                text_color = 0x0000FF;  // Blue for headings
            }
            
            // Render text (simple character by character)
            uint32 char_x = box->x;
            uint32 char_y = box->y;
            const char* text = box->node->text;
            
            for (uint32 i = 0; text[i] && i < 255; i++) {
                if (text[i] != ' ') {
                    render_char(fb, text[i], char_x, char_y, text_color);
                }
                char_x += 7;  // Character width + spacing
                
                // Line wrap (simple)
                if (char_x + 7 > box->x + box->width) {
                    char_x = box->x;
                    char_y += 9;  // Character height + spacing
                }
            }
        }
        
        box = box->next;
    }
}

#include "gfx_protocol.h"
#include "graphics.h"
#include "kheap.h"
#include "string.h"
#include "video.h"
#include "process.h"
#include "ipc.h"
#include "desktop.h"

/* Server window structure */
typedef struct {
    uint32_t id;
    uint32_t owner_pid;
    int32_t x, y;
    uint32_t width, height;
    uint32_t flags;
    char title[32];
    uint8_t* buffer;         /* Window pixel buffer */
    uint8_t visible;
    uint8_t focused;
    uint8_t damaged;         /* Needs redraw */
} gfx_window_t;

/* Global server state */
static struct {
    gfx_window_t windows[GFX_MAX_WINDOWS];
    uint32_t next_window_id;
    uint32_t focused_window;
    uint8_t initialized;
} g_gfx_server;

/* Find window by ID */
static gfx_window_t* find_window(uint32_t window_id) {
    for (int i = 0; i < GFX_MAX_WINDOWS; i++) {
        if (g_gfx_server.windows[i].id == window_id && g_gfx_server.windows[i].buffer) {
            return &g_gfx_server.windows[i];
        }
    }
    return NULL;
}

/* Find free window slot */
static gfx_window_t* find_free_window(void) {
    for (int i = 0; i < GFX_MAX_WINDOWS; i++) {
        if (g_gfx_server.windows[i].id == 0 || !g_gfx_server.windows[i].buffer) {
            return &g_gfx_server.windows[i];
        }
    }
    return NULL;
}

/* Draw a window to the screen */
static void gfx_draw_window_to_screen(gfx_window_t* win) {
    if (!win || !win->visible || !win->buffer) {
        return;
    }
    
    /* Draw window frame */
    draw_filled_rect(win->x, win->y, win->width, 18, 
                     win->focused ? COLOR_BLUE : COLOR_DARK_GRAY);
    draw_filled_rect(win->x, win->y + 18, win->width, win->height - 18, COLOR_LIGHT_GRAY);
    draw_rect(win->x, win->y, win->width, win->height, 
              win->focused ? COLOR_LIGHT_CYAN : COLOR_DARK_GRAY);
    
    /* Draw title */
    draw_text(win->x + 4, win->y + 4, win->title, COLOR_WHITE);
    
    /* Draw close button */
    draw_filled_rect(win->x + win->width - 14, win->y + 2, 12, 12, COLOR_RED);
    draw_text(win->x + win->width - 11, win->y + 4, "X", COLOR_WHITE);
    
    /* Draw content from buffer */
    uint32_t content_y = win->y + 20;
    uint32_t content_w = win->width - 4;
    uint32_t content_h = win->height - 24;
    
    for (uint32_t y = 0; y < content_h; y++) {
        for (uint32_t x = 0; x < content_w; x++) {
            uint32_t buf_idx = y * content_w + x;
            if (buf_idx < win->width * win->height) {
                draw_pixel(win->x + 2 + x, content_y + y, win->buffer[buf_idx]);
            }
        }
    }
}

/* Handle create window request */
static void handle_create_window(gfx_message_t* msg, gfx_response_t* resp) {
    gfx_create_window_t* req = &msg->data.create;
    
    gfx_window_t* win = find_free_window();
    if (!win) {
        resp->result = GFX_ERR_OUT_OF_MEMORY;
        return;
    }
    
    /* Allocate window buffer */
    uint32_t buf_size = req->width * req->height;
    win->buffer = (uint8_t*)kmalloc(buf_size);
    if (!win->buffer) {
        resp->result = GFX_ERR_OUT_OF_MEMORY;
        return;
    }
    
    /* Initialize window */
    win->id = g_gfx_server.next_window_id++;
    win->owner_pid = msg->sender_pid;
    win->x = req->x;
    win->y = req->y;
    win->width = req->width;
    win->height = req->height;
    win->flags = req->flags;
    win->visible = (req->flags & GFX_WINDOW_VISIBLE) ? 1 : 0;
    win->focused = 0;
    win->damaged = 1;
    strncpy(win->title, req->title, sizeof(win->title) - 1);
    
    /* Clear buffer */
    memset(win->buffer, COLOR_WHITE, buf_size);
    
    resp->result = GFX_OK;
    resp->data = win->id;
    
    debug_print("GFX: created window ");
    debug_print_hex(win->id);
    debug_print(" for PID ");
    debug_print_hex(msg->sender_pid);
    debug_print("\n");
}

/* Handle destroy window request */
static void handle_destroy_window(gfx_message_t* msg, gfx_response_t* resp) {
    gfx_window_t* win = find_window(msg->data.window_id);
    
    if (!win) {
        resp->result = GFX_ERR_INVALID_WINDOW;
        return;
    }
    
    /* Free buffer */
    if (win->buffer) {
        kfree(win->buffer);
        win->buffer = NULL;
    }
    
    win->id = 0;
    win->owner_pid = 0;
    
    resp->result = GFX_OK;
    
    debug_print("GFX: destroyed window ");
    debug_print_hex(msg->data.window_id);
    debug_print("\n");
}

/* Handle draw rectangle request */
static void handle_draw_rect(gfx_message_t* msg, gfx_response_t* resp, int filled) {
    gfx_draw_rect_t* req = &msg->data.rect;
    
    gfx_window_t* win = find_window(req->window_id);
    if (!win || !win->buffer) {
        resp->result = GFX_ERR_INVALID_WINDOW;
        return;
    }
    
    /* Draw to window buffer */
    for (uint32_t y = req->y; y < req->y + req->height && y < win->height; y++) {
        for (uint32_t x = req->x; x < req->x + req->width && x < win->width; x++) {
            /* For outline, only draw edges */
            if (!filled && 
                x != req->x && x != req->x + req->width - 1 &&
                y != req->y && y != req->y + req->height - 1) {
                continue;
            }
            win->buffer[y * win->width + x] = req->color;
        }
    }
    
    win->damaged = 1;
    resp->result = GFX_OK;
}

/* Handle draw text request */
static void handle_draw_text(gfx_message_t* msg, gfx_response_t* resp) {
    gfx_draw_text_t* req = &msg->data.text;
    
    gfx_window_t* win = find_window(req->window_id);
    if (!win || !win->buffer) {
        resp->result = GFX_ERR_INVALID_WINDOW;
        return;
    }
    
    /* Simple bitmap font (5x7) - just write ASCII directly for now */
    uint32_t x = req->x;
    uint32_t y = req->y;
    
    for (const char* c = req->text; *c && x < win->width - 6; c++) {
        /* Write character directly to buffer (simplified) */
        if (y < win->height - 8) {
            win->buffer[y * win->width + x] = req->color;
        }
        x += 6;
    }
    
    win->damaged = 1;
    resp->result = GFX_OK;
}

/* Handle present request */
static void handle_present(gfx_message_t* msg, gfx_response_t* resp) {
    gfx_window_t* win = find_window(msg->data.window_id);
    
    if (!win) {
        resp->result = GFX_ERR_INVALID_WINDOW;
        return;
    }
    
    win->damaged = 1;
    resp->result = GFX_OK;
}

/* Process incoming message */
static void process_message(gfx_message_t* msg, gfx_response_t* resp) {
    resp->request_id = msg->type;
    resp->result = GFX_OK;
    resp->data = 0;
    
    switch (msg->type) {
        case GFX_MSG_CREATE_WINDOW:
            handle_create_window(msg, resp);
            break;
            
        case GFX_MSG_DESTROY_WINDOW:
            handle_destroy_window(msg, resp);
            break;
            
        case GFX_MSG_SHOW_WINDOW: {
            gfx_window_t* win = find_window(msg->data.window_id);
            if (win) {
                win->visible = 1;
                win->damaged = 1;
            }
            break;
        }
            
        case GFX_MSG_HIDE_WINDOW: {
            gfx_window_t* win = find_window(msg->data.window_id);
            if (win) {
                win->visible = 0;
            }
            break;
        }
            
        case GFX_MSG_CLEAR: {
            gfx_window_t* win = find_window(msg->data.rect.window_id);
            if (win && win->buffer) {
                memset(win->buffer, msg->data.rect.color, win->width * win->height);
                win->damaged = 1;
            }
            break;
        }
            
        case GFX_MSG_DRAW_RECT:
            handle_draw_rect(msg, resp, 0);
            break;
            
        case GFX_MSG_FILL_RECT:
            handle_draw_rect(msg, resp, 1);
            break;
            
        case GFX_MSG_DRAW_TEXT:
            handle_draw_text(msg, resp);
            break;
            
        case GFX_MSG_PRESENT:
            handle_present(msg, resp);
            break;
            
        case GFX_MSG_SET_TITLE: {
            gfx_window_t* win = find_window(msg->data.text.window_id);
            if (win) {
                strncpy(win->title, msg->data.text.text, sizeof(win->title) - 1);
                win->damaged = 1;
            }
            break;
        }
            
        default:
            resp->result = GFX_ERR_INVALID_PARAM;
            break;
    }
}

/* Redraw all windows */
static void redraw_all(void) {
    /* Clear screen with desktop color */
    graphics_clear_screen(COLOR_BLUE);
    
    /* Draw all visible windows (back to front) */
    for (int i = GFX_MAX_WINDOWS - 1; i >= 0; i--) {
        gfx_window_t* win = &g_gfx_server.windows[i];
        if (win->id != 0 && win->visible && win->buffer) {
            gfx_draw_window_to_screen(win);
            win->damaged = 0;
        }
    }
    
    /* Present to screen */
    graphics_present();
}

/* Initialize graphics server */
int gfx_server_init(void) {
    if (g_gfx_server.initialized) {
        return 0;
    }
    
    memset(&g_gfx_server, 0, sizeof(g_gfx_server));
    g_gfx_server.next_window_id = 1;
    g_gfx_server.focused_window = 0;
    g_gfx_server.initialized = 1;
    
    debug_print("GFX Server: initialized\n");
    return 0;
}

/* Graphics server main loop */
void gfx_server_run(void) {
    gfx_message_t msg;
    gfx_response_t resp;
    
    gfx_server_init();
    
    debug_print("GFX Server: running\n");
    
    for (;;) {
        /* Process messages from all processes */
        process_t* current = process_get_current();
        if (current) {
            message_t ipc_msg;
            if (message_queue_dequeue(&current->inbox, &ipc_msg) == 0) {
                /* Convert IPC message to GFX message */
                msg.type = ipc_msg.type;
                msg.sender_pid = ipc_msg.sender_pid;
                /* Copy data from IPC message (data1 and data2) */
                memset(&msg.data, 0, sizeof(msg.data));
                memcpy(&msg.data.raw[0], &ipc_msg.data1, sizeof(uint32_t));
                memcpy(&msg.data.raw[4], &ipc_msg.data2, sizeof(uint32_t));
                
                /* Process the message */
                process_message(&msg, &resp);
            }
        }
        
        /* Redraw if any window is damaged */
        int needs_redraw = 0;
        for (int i = 0; i < GFX_MAX_WINDOWS; i++) {
            if (g_gfx_server.windows[i].damaged) {
                needs_redraw = 1;
                break;
            }
        }
        
        if (needs_redraw) {
            redraw_all();
        }
        
        /* Yield to other tasks */
        asm volatile("hlt");
    }
}
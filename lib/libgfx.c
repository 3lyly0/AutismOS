#include "gfx_protocol.h"
#include "ipc.h"
#include "string.h"
#include "process.h"

/* Client state */
static struct {
    uint32_t server_pid;
    uint32_t connected;
    uint32_t next_window_id;
} g_gfx_client;

/* Helper: send message and wait for response */
static int gfx_send_request(gfx_message_t* msg, gfx_response_t* resp) {
    if (!g_gfx_client.connected || g_gfx_client.server_pid == 0) {
        return GFX_ERR_NO_SERVER;
    }
    
    /* Send message to graphics server */
    message_t ipc_msg;
    ipc_msg.type = msg->type;
    ipc_msg.sender_pid = process_get_current()->pid;
    /* Copy data (only data1 and data2 available in message_t) */
    memcpy(&ipc_msg.data1, &msg->data, sizeof(uint32_t) * 2);
    
    process_t* server = process_find_by_pid(g_gfx_client.server_pid);
    if (!server) {
        return GFX_ERR_NO_SERVER;
    }
    
    int result = message_queue_enqueue(&server->inbox, &ipc_msg);
    
    if (result != 0) {
        return GFX_ERR_TIMEOUT;
    }
    
    /* For now, we don't wait for response in this simplified version */
    if (resp) {
        resp->result = GFX_OK;
        resp->data = 0;
    }
    
    return GFX_OK;
}

int gfx_connect(void) {
    /* In a real system, we'd find the server by name */
    /* For now, assume server PID is 1 */
    g_gfx_client.server_pid = 1;
    g_gfx_client.connected = 1;
    g_gfx_client.next_window_id = 1;
    
    return GFX_OK;
}

void gfx_disconnect(void) {
    g_gfx_client.connected = 0;
    g_gfx_client.server_pid = 0;
}

uint32_t gfx_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           const char* title, uint32_t flags) {
    if (!g_gfx_client.connected) {
        return 0;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_CREATE_WINDOW;
    msg.data.create.x = x;
    msg.data.create.y = y;
    msg.data.create.width = width;
    msg.data.create.height = height;
    msg.data.create.flags = flags;
    msg.data.create.buffer_size = width * height;
    
    if (title) {
        strncpy(msg.data.create.title, title, sizeof(msg.data.create.title) - 1);
    } else {
        msg.data.create.title[0] = '\0';
    }
    
    gfx_response_t resp;
    if (gfx_send_request(&msg, &resp) == GFX_OK) {
        return g_gfx_client.next_window_id++;
    }
    
    return 0;
}

int gfx_destroy_window(uint32_t window_id) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_DESTROY_WINDOW;
    msg.data.window_id = window_id;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_clear(uint32_t window_id, uint8_t color) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_CLEAR;
    msg.data.rect.window_id = window_id;
    msg.data.rect.x = 0;
    msg.data.rect.y = 0;
    msg.data.rect.width = 0;
    msg.data.rect.height = 0;
    msg.data.rect.color = color;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_draw_rect(uint32_t window_id, int32_t x, int32_t y, 
                  uint32_t w, uint32_t h, uint8_t color) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_DRAW_RECT;
    msg.data.rect.window_id = window_id;
    msg.data.rect.x = x;
    msg.data.rect.y = y;
    msg.data.rect.width = w;
    msg.data.rect.height = h;
    msg.data.rect.color = color;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_fill_rect(uint32_t window_id, int32_t x, int32_t y,
                  uint32_t w, uint32_t h, uint8_t color) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_FILL_RECT;
    msg.data.rect.window_id = window_id;
    msg.data.rect.x = x;
    msg.data.rect.y = y;
    msg.data.rect.width = w;
    msg.data.rect.height = h;
    msg.data.rect.color = color;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_draw_line(uint32_t window_id, int32_t x0, int32_t y0,
                  int32_t x1, int32_t y1, uint8_t color) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_DRAW_LINE;
    msg.data.line.window_id = window_id;
    msg.data.line.x0 = x0;
    msg.data.line.y0 = y0;
    msg.data.line.x1 = x1;
    msg.data.line.y1 = y1;
    msg.data.line.color = color;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_draw_text(uint32_t window_id, int32_t x, int32_t y,
                  const char* text, uint8_t color) {
    if (!g_gfx_client.connected || window_id == 0 || !text) {
        return GFX_ERR_INVALID_PARAM;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_DRAW_TEXT;
    msg.data.text.window_id = window_id;
    msg.data.text.x = x;
    msg.data.text.y = y;
    msg.data.text.color = color;
    strncpy(msg.data.text.text, text, sizeof(msg.data.text.text) - 1);
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_present(uint32_t window_id) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_PRESENT;
    msg.data.window_id = window_id;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_poll_event(gfx_message_t* event, uint32_t timeout_ms) {
    if (!g_gfx_client.connected || !event) {
        return GFX_ERR_INVALID_PARAM;
    }
    
    /* In a real implementation, we'd wait for a message from the server */
    /* For now, return no events */
    (void)timeout_ms;
    return 0;
}

int gfx_set_visible(uint32_t window_id, int visible) {
    if (!g_gfx_client.connected || window_id == 0) {
        return GFX_ERR_INVALID_WINDOW;
    }
    
    gfx_message_t msg;
    msg.type = visible ? GFX_MSG_SHOW_WINDOW : GFX_MSG_HIDE_WINDOW;
    msg.data.window_id = window_id;
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}

int gfx_set_title(uint32_t window_id, const char* title) {
    if (!g_gfx_client.connected || window_id == 0 || !title) {
        return GFX_ERR_INVALID_PARAM;
    }
    
    gfx_message_t msg;
    msg.type = GFX_MSG_SET_TITLE;
    msg.data.text.window_id = window_id;
    strncpy(msg.data.text.text, title, sizeof(msg.data.text.text) - 1);
    
    gfx_response_t resp;
    return gfx_send_request(&msg, &resp);
}
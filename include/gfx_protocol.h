#ifndef GFX_PROTOCOL_H
#define GFX_PROTOCOL_H

#include <stdint.h>

/**
 * Graphics Server IPC Protocol
 * 
 * Defines the communication protocol between applications
 * and the graphics server process.
 * 
 * Applications send commands to the graphics server via IPC
 * to create windows, draw content, and receive input events.
 */

/* ============== Protocol Constants ============== */

#define GFX_SERVER_NAME    "gfx_server"
#define GFX_MAX_WINDOWS    16
#define GFX_MAX_EVENTS     32

/* Message types */
typedef enum {
    /* Window management */
    GFX_MSG_CREATE_WINDOW   = 0x0100,
    GFX_MSG_DESTROY_WINDOW  = 0x0101,
    GFX_MSG_SHOW_WINDOW     = 0x0102,
    GFX_MSG_HIDE_WINDOW     = 0x0103,
    GFX_MSG_MOVE_WINDOW     = 0x0104,
    GFX_MSG_RESIZE_WINDOW   = 0x0105,
    GFX_MSG_FOCUS_WINDOW    = 0x0106,
    GFX_MSG_SET_TITLE       = 0x0107,
    
    /* Drawing commands */
    GFX_MSG_CLEAR           = 0x0200,
    GFX_MSG_DRAW_PIXEL      = 0x0201,
    GFX_MSG_DRAW_LINE       = 0x0202,
    GFX_MSG_DRAW_RECT       = 0x0203,
    GFX_MSG_FILL_RECT       = 0x0204,
    GFX_MSG_DRAW_TEXT       = 0x0205,
    GFX_MSG_PRESENT         = 0x0206,
    GFX_MSG_COPY_RECT       = 0x0207,
    
    /* Input events (from server to client) */
    GFX_MSG_EVENT_KEY       = 0x0300,
    GFX_MSG_EVENT_MOUSE     = 0x0301,
    GFX_MSG_EVENT_FOCUS     = 0x0302,
    GFX_MSG_EVENT_CLOSE     = 0x0303,
    GFX_MSG_EVENT_RESIZE    = 0x0304,
    
    /* Server responses */
    GFX_MSG_RESPONSE        = 0x0400,
    GFX_MSG_ERROR           = 0x0401
} gfx_msg_type_t;

/* Error codes */
typedef enum {
    GFX_OK                  = 0,
    GFX_ERR_INVALID_WINDOW  = -1,
    GFX_ERR_OUT_OF_MEMORY   = -2,
    GFX_ERR_INVALID_PARAM   = -3,
    GFX_ERR_NO_SERVER       = -4,
    GFX_ERR_TIMEOUT         = -5,
    GFX_ERR_PERMISSION      = -6
} gfx_error_t;

/* Window flags */
typedef enum {
    GFX_WINDOW_NONE         = 0,
    GFX_WINDOW_VISIBLE      = 0x01,
    GFX_WINDOW_FOCUSABLE    = 0x02,
    GFX_WINDOW_RESIZABLE    = 0x04,
    GFX_WINDOW_DRAGGABLE    = 0x08,
    GFX_WINDOW_BORDERLESS   = 0x10,
    GFX_WINDOW_MODAL        = 0x20
} gfx_window_flags_t;

/* Input event types */
typedef enum {
    GFX_INPUT_KEY_PRESS     = 0,
    GFX_INPUT_KEY_RELEASE   = 1,
    GFX_INPUT_MOUSE_MOVE    = 2,
    GFX_INPUT_MOUSE_PRESS   = 3,
    GFX_INPUT_MOUSE_RELEASE = 4
} gfx_input_type_t;

/* Mouse buttons */
typedef enum {
    GFX_MOUSE_LEFT          = 0x01,
    GFX_MOUSE_RIGHT         = 0x02,
    GFX_MOUSE_MIDDLE        = 0x04
} gfx_mouse_button_t;

/* ============== Message Structures ============== */

/* Create window message */
typedef struct {
    uint32_t window_id;     /* Output: assigned window ID */
    int32_t x, y;           /* Position */
    uint32_t width, height; /* Size */
    uint32_t flags;         /* Window flags */
    char title[32];         /* Window title */
    uint32_t buffer_size;   /* Size of pixel buffer */
} gfx_create_window_t;

/* Draw rectangle message */
typedef struct {
    uint32_t window_id;
    int32_t x, y;
    uint32_t width, height;
    uint8_t color;
} gfx_draw_rect_t;

/* Draw line message */
typedef struct {
    uint32_t window_id;
    int32_t x0, y0, x1, y1;
    uint8_t color;
} gfx_draw_line_t;

/* Draw text message */
typedef struct {
    uint32_t window_id;
    int32_t x, y;
    char text[64];
    uint8_t color;
} gfx_draw_text_t;

/* Key event message */
typedef struct {
    uint32_t window_id;
    uint8_t key;            /* ASCII or scancode */
    uint8_t type;           /* press/release */
    uint8_t modifiers;      /* Shift, Ctrl, Alt flags */
} gfx_event_key_t;

/* Mouse event message */
typedef struct {
    uint32_t window_id;
    int32_t x, y;           /* Position relative to window */
    int32_t dx, dy;         /* Movement delta */
    uint8_t buttons;        /* Button state */
    uint8_t type;           /* move/press/release */
} gfx_event_mouse_t;

/* Focus event message */
typedef struct {
    uint32_t window_id;
    uint8_t gained;         /* 1 if gained, 0 if lost */
} gfx_event_focus_t;

/* Resize event message */
typedef struct {
    uint32_t window_id;
    uint32_t old_width, old_height;
    uint32_t new_width, new_height;
} gfx_event_resize_t;

/* Generic response message */
typedef struct {
    uint32_t request_id;
    int32_t result;         /* 0 on success, negative on error */
    uint32_t data;          /* Additional data */
} gfx_response_t;

/* ============== Graphics Message Union ============== */

typedef struct {
    uint32_t type;          /* Message type */
    uint32_t sender_pid;    /* Filled by IPC */
    union {
        gfx_create_window_t create;
        gfx_draw_rect_t rect;
        gfx_draw_line_t line;
        gfx_draw_text_t text;
        gfx_event_key_t key;
        gfx_event_mouse_t mouse;
        gfx_event_focus_t focus;
        gfx_event_resize_t resize;
        gfx_response_t response;
        uint32_t window_id;  /* For simple window operations */
        uint8_t raw[64];
    } data;
} gfx_message_t;

/* ============== Client API ============== */

/**
 * Connect to the graphics server
 * 
 * @return           0 on success, error code on failure
 */
int gfx_connect(void);

/**
 * Disconnect from the graphics server
 */
void gfx_disconnect(void);

/**
 * Create a new window
 * 
 * @param x          X position
 * @param y          Y position
 * @param width      Window width
 * @param height     Window height
 * @param title      Window title
 * @param flags      Window flags
 * @return           Window ID, or 0 on failure
 */
uint32_t gfx_create_window(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           const char* title, uint32_t flags);

/**
 * Destroy a window
 * 
 * @param window_id  Window to destroy
 * @return           0 on success, error code on failure
 */
int gfx_destroy_window(uint32_t window_id);

/**
 * Clear window to a color
 * 
 * @param window_id  Window to clear
 * @param color      Fill color
 */
int gfx_clear(uint32_t window_id, uint8_t color);

/**
 * Draw a rectangle outline
 * 
 * @param window_id  Target window
 * @param x, y       Position
 * @param w, h       Size
 * @param color      Rectangle color
 */
int gfx_draw_rect(uint32_t window_id, int32_t x, int32_t y, 
                  uint32_t w, uint32_t h, uint8_t color);

/**
 * Draw a filled rectangle
 * 
 * @param window_id  Target window
 * @param x, y       Position
 * @param w, h       Size
 * @param color      Fill color
 */
int gfx_fill_rect(uint32_t window_id, int32_t x, int32_t y,
                  uint32_t w, uint32_t h, uint8_t color);

/**
 * Draw a line
 * 
 * @param window_id  Target window
 * @param x0, y0     Start point
 * @param x1, y1     End point
 * @param color      Line color
 */
int gfx_draw_line(uint32_t window_id, int32_t x0, int32_t y0,
                  int32_t x1, int32_t y1, uint8_t color);

/**
 * Draw text
 * 
 * @param window_id  Target window
 * @param x, y       Position
 * @param text       Text to draw
 * @param color      Text color
 */
int gfx_draw_text(uint32_t window_id, int32_t x, int32_t y,
                  const char* text, uint8_t color);

/**
 * Present window buffer to screen
 * 
 * @param window_id  Window to present
 */
int gfx_present(uint32_t window_id);

/**
 * Get next event from server
 * 
 * @param event      Output event structure
 * @param timeout_ms Maximum time to wait (0 = no wait)
 * @return           1 if event received, 0 if timeout, negative on error
 */
int gfx_poll_event(gfx_message_t* event, uint32_t timeout_ms);

/**
 * Set window visibility
 * 
 * @param window_id  Target window
 * @param visible    1 to show, 0 to hide
 */
int gfx_set_visible(uint32_t window_id, int visible);

/**
 * Set window title
 * 
 * @param window_id  Target window
 * @param title      New title
 */
int gfx_set_title(uint32_t window_id, const char* title);

#endif /* GFX_PROTOCOL_H */
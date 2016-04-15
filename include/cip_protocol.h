#ifndef cip_protocol_h
#define cip_protocol_h

#include "cip_defs.h"

#define CIP_ATTR_PACKED __attribute__ ((__packed__))
#define CIP_SESSION_LENGTH 128


/* common data types */
/* result list */
enum CIP_RESULT {
    CIP_RESULT_SUCCESS,
    CIP_RESULT_FAIL,
    CIP_RESULT_NO_CHANNEL,
    CIP_RESULT_NEED_AUTH,
    CIP_RESULT_NEED_SESSION,
};

/* channel types */
enum CIP_CHANNEL {
    CIP_CHANNEL_MASTER,
    CIP_CHANNEL_EVENT,
    CIP_CHANNEL_DISPLAY,
    CIP_CHANNEL_DATA,
};

typedef struct CIP_ATTR_PACKED {
    u8 major_version;
    u8 minor_version;
} version_t;

typedef struct CIP_ATTR_PACKED {
    version_t version;
    u8 channel_type;
    u8 encrypt; /* bool, encrypt or not */
} cip_message_connect_t;

typedef struct CIP_ATTR_PACKED {
    u8 result;
} cip_message_connect_reply_t;

/* end common data types */

/* master channel data types */
typedef struct {
    
} cip_request_token_get;

/* end master channel data types */

#define CIP_KEY_CODE_MOUSE_LEFT_DOWN 1
#define CIP_KEY_CODE_MOUSE_LEFT_UP 2

enum CIP_EVENT {
    CIP_EVENT_WINDOW_CREATE,
    CIP_EVENT_WINDOW_DESTROY,
    CIP_EVENT_WINDOW_SHOW,
    CIP_EVENT_WINDOW_HIDE,
    CIP_EVENT_WINDOW_CONFIGURE = 4,
    CIP_EVENT_MOUSE_MOVE,
    CIP_EVENT_MOUSE_DOWN,
    CIP_EVENT_MOUSE_UP,
    CIP_EVENT_KEY_DOWN = 8,
    CIP_EVENT_KEY_UP,
    CIP_EVENT_WINDOW_MOVE,
    CIP_EVENT_WINDOW_RESIZE,
};

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    i16 x;
    i16 y;
    u16 width;
    u16 height;
    u8 bare;
} cip_event_window_create_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
} cip_event_window_destroy_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
} cip_event_window_hide_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u8 bare;
    u32 wid;
} cip_event_window_show_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    i16 x;
    i16 y;
    u16 width;
    u16 height;
    u32 above;
    u8 bare;
} cip_event_window_configure_t;

typedef struct CIP_ATTR_PACKED {
    u32 wid;
    u32 length;
} cip_event_window_frame_t;

/* bellow is event from client */
typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    i16 x;
    i16 y;
} cip_event_mouse_move_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    u8 code;
} cip_event_mouse_down_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    u8 code;
} cip_event_mouse_up_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    u8 code;
} cip_event_key_down_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    u8 code;
} cip_event_key_up_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    i16 x;
    i16 y;
} cip_event_window_move_t;

typedef struct CIP_ATTR_PACKED {
    u8 type;
    u32 wid;
    u16 width;
    u16 height;
} cip_event_window_resize_t;


typedef union CIP_ATTR_PACKED {
    u8 type;
    cip_event_window_create_t window_create;
    cip_event_window_destroy_t window_destroy;
    cip_event_window_show_t window_show;
    cip_event_window_hide_t window_hide;
    cip_event_window_configure_t window_configure;
    cip_event_mouse_move_t mouse_move;
    cip_event_mouse_down_t mouse_down;
    cip_event_mouse_up_t mouse_up;
    cip_event_key_down_t key_down;
    cip_event_key_up_t key_up;
    cip_event_window_move_t window_move;
    cip_event_window_resize_t window_resize;
} cip_event_t;



#endif
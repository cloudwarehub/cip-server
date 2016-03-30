#ifndef cip_protocol_h
#define cip_protocol_h

#include "cip_defs.h"

#define CIP_ATTR_PACKED __attribute__ ((__packed__))
#define CIP_SESSION_LENGTH 128

enum CIP_RESULT {
    CIP_RESULT_SUCCESS,
    CIP_RESULT_FAIL,
    CIP_RESULT_NO_CHANNEL,
    CIP_RESULT_NEED_AUTH,
    CIP_RESULT_NEED_SESSION,
};

enum CIP_CHANNEL {
    CIP_CHANNEL_MASTER,
    CIP_CHANNEL_EVENT,
    CIP_CHANNEL_DISPLAY,
    CIP_CHANNEL_DATA,
};

typedef struct {
    u8 major_version;
    u8 minor_version;
} version_t;



typedef struct {
    version_t version;
    u8 channel_type;
    u8 encrypt; /* bool, encrypt or not */
} cip_message_connect_t;

typedef struct {
    u8 result;
} cip_message_connect_reply_t;

typedef struct {
    u16 type;
    u32 length;
} cip_message_t;

enum CIP_EVENT {
    CIP_EVENT_WINDOW_CREATE,
    CIP_EVENT_WINDOW_DESTROY,
    CIP_EVENT_WINDOW_SHOW,
    CIP_EVENT_WINDOW_HIDE,
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
    u32 wid;
    u32 length;
} cip_event_window_frame_t;

typedef union CIP_ATTR_PACKED {
    u8 type;
    cip_event_window_create_t window_create;
    cip_event_window_destroy_t window_destroy;
    cip_event_window_show_t window_show;
    cip_event_window_hide_t window_hide;
} cip_event_t;

#endif
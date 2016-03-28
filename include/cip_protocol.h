#ifndef cip_protocol_h
#define cip_protocol_h

#include "cip_defs.h"

enum CIP_RESULT {
    CIP_RESULT_SUCCESS,
    CIP_RESULT_FAIL,
    CIP_RESULT_NEED_AUTH,
    CIP_RESULT_NO_CHANNEL,
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
    union {
        version_t version;
        u16 session_length;
    };
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

#endif
#ifndef cip_protocol_h
#define cip_protocol_h

#include "cip_defs.h"

enum CIP_CHANNEL {
    CIP_CHANNEL_MASTER,
    CIP_CHANNEL_EVENT,
    CIP_CHANNEL_DISPLAY,
    CIP_CHANNEL_DATA,
};

typedef struct {
    u8 version;
    u8 channel_type;
    u16 session_length; /* also used for session_length */
    /* token/session */
} cip_message_connect_t;

typedef struct {
    u8 success;
    u8 _pad[3];
    //u16 session_length;
    /* session */
} cip_message_connect_reply_t;

enum CIP_EVENT {
    CIP_EVENT_WINDOW_CREATE,
    CIP_EVENT_WINDOW_DESTROY,
    CIP_EVENT_WINDOW_SHOW,
    CIP_EVENT_WINDOW_HIDE,
};

#endif
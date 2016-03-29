//
//  cip_server.h
//  cip-server
//
//  Created by gd on 16/3/28.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#ifndef cip_server_h
#define cip_server_h

#include <stdint.h>
#include <x264.h>
#include <xcb/xcb.h>
#include "list.h"
#include "cip_channel.h"

#define SESSION_LENGTH 128

typedef struct {
    list_head_t sessions;
    list_head_t windows;
    xcb_connection_t *xconn;
} cip_context_t;

typedef struct {
    list_head_t list_node;
    cip_channel_t *master_channel;
    cip_channel_t *event_channel;
    cip_channel_t *display_channel;
    char session[SESSION_LENGTH];
} cip_session_t;

/* used for non session server, every channel has a dummy session */
typedef struct {
    list_head_t list_node;
    cip_channel_t *channel;
} cip_session_dummy_t;

typedef struct {
    list_head_t list_node;
    int wid;
    x264_param_t param;
    x264_t *encoder;
    x264_picture_t pic;
    int i_frame;
    uint16_t width;
    uint16_t height;
    uint16_t even_width;
    uint16_t even_height;
} cip_window_t;

typedef struct {
    list_head_t list_node;
    cip_event_t event;
} cip_event_node_t;

#endif /* cip_server_h */

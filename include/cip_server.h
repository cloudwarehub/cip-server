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

typedef struct {
    list_head_t sessions;
    list_head_t windows;
    xcb_connection_t *xconn;
} cip_context_t;

typedef struct {
    list_head_t list_node;
    int channel_master_sock;
    int channel_event_sock;
    int channel_display_sock;
    char session[37];
} cip_session_t;

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

#endif /* cip_server_h */

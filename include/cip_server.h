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
#include <xcb/xcb.h>
#include <uv.h>
#include "list.h"
#include "cip_channel.h"

typedef struct {
    list_head_t sessions;
    list_head_t windows;
    xcb_connection_t *xconn;
} cip_context_t;

typedef struct {
    uv_write_t req;
    list_head_t list_node;
    enum CIP_CHANNEL channel_type;

    uv_buf_t buf;
} write_req_t;

void after_write(uv_write_t *req, int status);

#endif /* cip_server_h */

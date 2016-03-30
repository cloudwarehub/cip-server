//
//  cip_session.h
//  cip-server
//
//  Created by gd on 16/3/30.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#ifndef cip_session_h
#define cip_session_h

#include "list.h"
#include "cip_channel.h"
#include "cip_protocol.h"

typedef struct {
    list_head_t list_node;
    cip_channel_t *master_channel;
    cip_channel_t *event_channel;
    cip_channel_t *display_channel;
    char session[CIP_SESSION_LENGTH];
} cip_session_t;

void cip_session_init(cip_session_t *session);

#endif /* cip_session_h */

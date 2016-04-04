//
//  cip_channel.c
//  cip-server
//
//  Created by gd on 16/4/4.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "cip_channel.h"
#include "cip_common.h"
#include "cip_session.h"
#include "cip_server.h"

extern cip_context_t cip_context;

int need_session = 0;


/* recover window state to event channel */
void recover_state(cip_channel_t *channel)
{
    write_req_t *wr;
    cip_window_t *iter;
    list_for_each_entry(iter, &cip_context.windows, list_node) {
        /* send create window event */
        wr = (write_req_t*) malloc(sizeof(write_req_t));
        cip_event_window_create_t *cewc = malloc(sizeof(cip_event_window_create_t));
        cewc->type = CIP_EVENT_WINDOW_CREATE;
        cewc->wid = iter->wid;
        cewc->x = iter->x;
        cewc->y = iter->y;
        cewc->width = iter->width;
        cewc->height = iter->height;
        cewc->bare = iter->bare;
        wr = malloc(sizeof(write_req_t));
        wr->buf = uv_buf_init((char*)cewc, sizeof(*cewc));
        wr->buf = uv_buf_init((char*)cewc, sizeof(cip_event_window_create_t));
        uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
        
        /* if viewable send show window event */
        if (iter->viewable) {
            wr = malloc(sizeof(write_req_t));
            cip_event_window_show_t *cews = malloc(sizeof(cip_event_window_show_t));
            cews->type = CIP_EVENT_WINDOW_SHOW;
            
            cews->wid = iter->wid;
            cews->bare = iter->bare;
            wr->buf = uv_buf_init((char*)cews, sizeof(*cews));
            uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
        }
    }
}

void cip_channel_handle(cip_channel_t *channel)
{
    if (!channel->connected) {
        if (ringbuf_bytes_used(channel->rx_ring) < sizeof(cip_message_connect_t)) {
            return;
        }
        
        write_req_t *wr;
        cip_message_connect_reply_t *msg_conn_rpl;
        cip_message_connect_t *msg_conn = (cip_message_connect_t*)ringbuf_head(channel->rx_ring);
        switch (msg_conn->channel_type) {
            case CIP_CHANNEL_MASTER:
                printf("master channel\n");
                cip_check_version(msg_conn->version.major_version, msg_conn->version.minor_version);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_MASTER;
                
                /* create new session and add to cip_context */
                cip_session_t *cip_session = malloc(sizeof(cip_session_t));
                cip_session_init(cip_session);
                cip_session->master_channel = channel;
                /* generate random session string */
                rand_string(cip_session->session, sizeof(cip_session->session));
                list_add_tail(&cip_session->list_node, &cip_context.sessions);
                printf("session: %s\n", cip_session->session);
                
                /* write connect result back */
                wr = (write_req_t*) malloc(sizeof(write_req_t));
                msg_conn_rpl = malloc(sizeof(cip_message_connect_reply_t));
                msg_conn_rpl->result = CIP_RESULT_SUCCESS;
                wr->buf = uv_buf_init((char*)msg_conn_rpl, sizeof(cip_message_connect_reply_t));
                uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
                break;
                
            case CIP_CHANNEL_EVENT:
                printf("event channel\n");
                
                /* write connect result back */
                wr = (write_req_t*) malloc(sizeof(write_req_t));
                msg_conn_rpl = malloc(sizeof(cip_message_connect_reply_t));
                if (need_session) {
                    msg_conn_rpl->result = CIP_RESULT_NEED_SESSION;
                } else {
                    msg_conn_rpl->result = CIP_RESULT_SUCCESS;
                    cip_session_t *cip_session = malloc(sizeof(cip_session_t));
                    cip_session_init(cip_session);
                    cip_session->event_channel = channel;
                    list_add_tail(&cip_session->list_node, &cip_context.sessions);
                }
                wr->buf = uv_buf_init((char*)msg_conn_rpl, sizeof(cip_message_connect_reply_t));
                uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_EVENT;
                
                /* recover window state on channel established */
                recover_state(channel);
                break;
                
            case CIP_CHANNEL_DISPLAY:
                printf("display channel\n");
                
                /* write connect result back */
                wr = (write_req_t*) malloc(sizeof(write_req_t));
                msg_conn_rpl = malloc(sizeof(cip_message_connect_reply_t));
                if (need_session) {
                    msg_conn_rpl->result = CIP_RESULT_NEED_SESSION;
                } else {
                    msg_conn_rpl->result = CIP_RESULT_SUCCESS;
                    cip_session_t *cip_session = malloc(sizeof(cip_session_t));
                    cip_session_init(cip_session);
                    cip_session->display_channel = channel;
                    list_add_tail(&cip_session->list_node, &cip_context.sessions);
                }
                wr->buf = uv_buf_init((char*)msg_conn_rpl, sizeof(cip_message_connect_reply_t));
                uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_DISPLAY;
                
                break;
                
            default:
                break;
        }
    } else { /* channel already connected */
        /* TODO: check nread size */
        
        if (channel->type == CIP_CHANNEL_MASTER) {
            //cip_message_t *msg = (cip_message_t*)buf->base;
        } else if (channel->type == CIP_CHANNEL_EVENT) {
            //cip_event_t *m = (cip_event_t*)buf->base;
        }
    }
}
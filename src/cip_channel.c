//
//  cip_channel.c
//  cip-server
//
//  Created by gd on 16/4/4.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xtest.h>
#include "list.h"
#include "cip_window.h"
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
    if (channel->type == CIP_CHANNEL_EVENT) {
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
    } else if (channel->type == CIP_CHANNEL_DISPLAY) {
        list_for_each_entry(iter, &cip_context.windows, list_node) {
            cip_window_stream_reset(iter);
        }
    }
    
}

uint8_t map_key_code(uint8_t code)
{
    return code;
}

void handle_event(cip_event_t *event)
{
    switch (event->type) {
        case CIP_EVENT_MOUSE_MOVE:
            xcb_warp_pointer(cip_context.xconn, XCB_NONE, event->mouse_move.wid, 0, 0, 0, 0, event->mouse_move.x, event->mouse_move.y);
            xcb_flush(cip_context.xconn);
            break;
        case CIP_EVENT_MOUSE_DOWN:
            xcb_test_fake_input(cip_context.xconn, XCB_BUTTON_PRESS, event->mouse_down.code, XCB_CURRENT_TIME, event->mouse_down.wid, 0, 0, 0);
            xcb_flush(cip_context.xconn);
            break;
        case CIP_EVENT_MOUSE_UP:
            xcb_test_fake_input(cip_context.xconn, XCB_BUTTON_RELEASE, event->mouse_up.code, XCB_CURRENT_TIME, event->mouse_up.wid, 0, 0, 0);
            xcb_flush(cip_context.xconn);
            break;
        case CIP_EVENT_KEY_DOWN: {
            uint8_t code = map_key_code(event->key_down.code);
            xcb_test_fake_input(cip_context.xconn, XCB_KEY_PRESS, code, XCB_CURRENT_TIME, event->key_down.wid, 0, 0, 0 );
            xcb_flush(cip_context.xconn);
            break;
        }
        case CIP_EVENT_KEY_UP: {
            uint8_t code = map_key_code(event->key_down.code);
            xcb_test_fake_input(cip_context.xconn, XCB_KEY_RELEASE, code, XCB_CURRENT_TIME, event->key_down.wid, 0, 0, 0 );
            xcb_flush(cip_context.xconn);
            break;
        }
        case CIP_EVENT_WINDOW_MOVE: {
            cip_window_t *window = find_window(event->window_move.wid, &cip_context.windows);
            window->x = event->window_move.x;
            window->y = event->window_move.y;
            u32 values[] = {window->x, window->y};
            xcb_configure_window(cip_context.xconn, window->wid, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
            xcb_flush(cip_context.xconn);
            break;
        }
        case CIP_EVENT_WINDOW_RESIZE: {
            cip_window_t *window = find_window(event->window_move.wid, &cip_context.windows);
            window->width = event->window_resize.width;
            window->height = event->window_resize.height;
            u32 values[] = {window->width, window->height};
            xcb_configure_window(cip_context.xconn, window->wid, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
            xcb_flush(cip_context.xconn);
            /* h264 stream resize */
            cip_window_stream_reset(window);
            break;
        }
        case CIP_EVENT_WINDOW_DESTROY: {
            cip_window_t *window = find_window(event->window_move.wid, &cip_context.windows);
            xcb_destroy_window(cip_context.xconn, window->wid);
            break;
        }
        case CIP_EVENT_WINDOW_SHOW_READY: {
            cip_window_t *window = find_window(event->window_frame_listen.wid, &cip_context.windows);
            //cip_window_stream_reset(window);
            cip_window_frame_send(window->wid, 1);
            break;
        }
        default:
            break;
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
        cip_message_connect_t msg_conn;
        ringbuf_memcpy_from(&msg_conn, channel->rx_ring, sizeof(cip_message_connect_t));
        
        switch (msg_conn.channel_type) {
            case CIP_CHANNEL_MASTER:
                printf("master channel\n");
                cip_check_version(msg_conn.version.major_version, msg_conn.version.minor_version);
                
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
                
                channel->session = cip_session;
                
                /* write connect result back */
                wr = (write_req_t*) malloc(sizeof(write_req_t));
                /* send result and session */
                char *buf = malloc(sizeof(cip_message_connect_reply_t) + CIP_SESSION_LENGTH);
                msg_conn_rpl = (cip_message_connect_reply_t*)buf;
                msg_conn_rpl->result = CIP_RESULT_SUCCESS;
                memcpy(buf + sizeof(cip_message_connect_reply_t), cip_session->session, CIP_SESSION_LENGTH);
                wr->buf = uv_buf_init((char*)buf, sizeof(cip_message_connect_reply_t) + CIP_SESSION_LENGTH);
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
                    
                    channel->session = cip_session;
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
                    
                    channel->session = cip_session;
                }
                wr->buf = uv_buf_init((char*)msg_conn_rpl, sizeof(cip_message_connect_reply_t));
                uv_write(&wr->req, channel->client, &wr->buf, 1, after_write);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_DISPLAY;
                
                /* recover display info */
                recover_state(channel);

                break;
                
            default:
                break;
        }
    } else { /* channel already connected */
        
        if (channel->type == CIP_CHANNEL_MASTER) {
            //cip_message_t *msg = (cip_message_t*)buf->base;
        } else if (channel->type == CIP_CHANNEL_EVENT) {
            uint8_t ev_type = *(char*)ringbuf_tail(channel->rx_ring);
            int expect_size = get_size_by_type(ev_type);
            while (ringbuf_bytes_used(channel->rx_ring) >= expect_size) {
                cip_event_t event;
                ringbuf_memcpy_from(&event, channel->rx_ring, expect_size);
                
                /* handle event */
                handle_event(&event);
                
                if (ringbuf_is_empty(channel->rx_ring)) {
                    break;
                } else {
                    ev_type = *(char*)ringbuf_tail(channel->rx_ring);
                    expect_size = get_size_by_type(ev_type);
                }
            }
        }
    }
}

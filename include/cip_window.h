//
//  cip_window.h
//  cip-server
//
//  Created by gd on 16/3/28.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#ifndef cip_window_h
#define cip_window_h

#include <x264.h>
#include <uv.h>
#include "list.h"

typedef struct {
    list_head_t list_node;
    int wid;
    x264_param_t param;
    x264_t *encoder;
    x264_picture_t pic;
    int i_frame;
    uint8_t bare;
    uint8_t viewable;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t even_width;
    uint16_t even_height;
    uv_mutex_t streamlock;
    pthread_mutex_t mutex;
    int stream_ready;
    
    uv_async_t async;
    int force_keyframe;
    //uv_work_t stream_thread;
    // uv_loop_t *thread_loop;
} cip_window_t;

void cip_window_frame_send(uv_async_t *async);
int cip_window_stream_init(cip_window_t *window);
void cip_window_stream_reset(cip_window_t *window);

#endif /* cip_window_h */

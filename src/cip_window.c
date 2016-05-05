//
//  cip_window.c
//  cip-server
//
//  Created by gd on 16/3/28.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#include <stdint.h>
#include <x264.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cip_window.h"
#include "cip_server.h"
#include "cip_protocol.h"
#include "list.h"

typedef uint8_t uint8;

extern cip_context_t cip_context;
extern uv_async_t async;

void toeven(size_t *num)
{
    if (*num % 2) {
        *num = *num - 1;
    }
}

static __inline int RGBToY(uint8 r, uint8 g, uint8 b) {
    return (66 * r + 129 * g +  25 * b + 0x1080) >> 8;
}

static __inline int RGBToU(uint8 r, uint8 g, uint8 b) {
    return (112 * b - 74 * g - 38 * r + 0x8080) >> 8;
}
static __inline int RGBToV(uint8 r, uint8 g, uint8 b) {
    return (112 * r - 94 * g - 18 * b + 0x8080) >> 8;
}

#define MAKEROWY(NAME, R, G, B, BPP) \
    void NAME ## ToYRow_C(const uint8* src_argb0, uint8* dst_y, int width) {       \
    int x;                                                                       \
        for (x = 0; x < width; ++x) {                                                \
            dst_y[0] = RGBToY(src_argb0[R], src_argb0[G], src_argb0[B]);               \
            src_argb0 += BPP;                                                          \
            dst_y += 1;                                                                \
        }                                                                            \
    }                                                                              \
    void NAME ## ToUVRow_C(const uint8* src_rgb0, int src_stride_rgb,              \
    uint8* dst_u, uint8* dst_v, int width) {                \
        const uint8* src_rgb1 = src_rgb0 + src_stride_rgb;                           \
        int x;                                                                       \
        for (x = 0; x < width - 1; x += 2) {                                         \
        uint8 ab = (src_rgb0[B] + src_rgb0[B + BPP] +                              \
        src_rgb1[B] + src_rgb1[B + BPP]) >> 2;                          \
        uint8 ag = (src_rgb0[G] + src_rgb0[G + BPP] +                              \
        src_rgb1[G] + src_rgb1[G + BPP]) >> 2;                          \
        uint8 ar = (src_rgb0[R] + src_rgb0[R + BPP] +                              \
        src_rgb1[R] + src_rgb1[R + BPP]) >> 2;                          \
        dst_u[0] = RGBToU(ar, ag, ab);                                             \
        dst_v[0] = RGBToV(ar, ag, ab);                                             \
        src_rgb0 += BPP * 2;                                                       \
        src_rgb1 += BPP * 2;                                                       \
        dst_u += 1;                                                                \
        dst_v += 1;                                                                \
    }                                                                            \
    if (width & 1) {                                                             \
        uint8 ab = (src_rgb0[B] + src_rgb1[B]) >> 1;                               \
        uint8 ag = (src_rgb0[G] + src_rgb1[G]) >> 1;                               \
        uint8 ar = (src_rgb0[R] + src_rgb1[R]) >> 1;                               \
        dst_u[0] = RGBToU(ar, ag, ab);                                             \
        dst_v[0] = RGBToV(ar, ag, ab);                                             \
    }                                                                            \
}

MAKEROWY(ARGB, 2, 1, 0, 4)

int ARGBToI420(const uint8_t* src_argb, int src_stride_argb,
               uint8_t* dst_y, int dst_stride_y,
               uint8_t* dst_u, int dst_stride_u,
               uint8_t* dst_v, int dst_stride_v,
               int width, int height) {
    int y;
    void (*ARGBToUVRow)(const uint8_t* src_argb0, int src_stride_argb,
                        uint8* dst_u, uint8_t* dst_v, int width) = ARGBToUVRow_C;
    void (*ARGBToYRow)(const uint8* src_argb, uint8_t* dst_y, int pix) =
    ARGBToYRow_C;
    if (!src_argb ||
        !dst_y || !dst_u || !dst_v ||
        width <= 0 || height == 0) {
        return -1;
    }
    // Negative height means invert the image.
    if (height < 0) {
        height = -height;
        src_argb = src_argb + (height - 1) * src_stride_argb;
        src_stride_argb = -src_stride_argb;
    }
    
    for (y = 0; y < height - 1; y += 2) {
        ARGBToUVRow(src_argb, src_stride_argb, dst_u, dst_v, width);
        ARGBToYRow(src_argb, dst_y, width);
        ARGBToYRow(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
        src_argb += src_stride_argb * 2;
        dst_y += dst_stride_y * 2;
        dst_u += dst_stride_u;
        dst_v += dst_stride_v;
    }
    if (height & 1) {
        ARGBToUVRow(src_argb, 0, dst_u, dst_v, width);
        ARGBToYRow(src_argb, dst_y, width);
    }
    return 0;
}

int cip_window_stream_init(cip_window_t *window)
{
    window->async.data = window;
    uv_async_init(cip_context.loop, &window->async, cip_window_frame_send);
    
    uint16_t width = window->width;
    uint16_t height = window->height;
    
    toeven((size_t*)&width);
    toeven((size_t*)&height);
    window->even_width = width;
    window->even_height = height;
    
    
    if (height < 4 || width < 4) { /* image is too small to stream */
        return 1;
    }
    x264_param_t *param = &window->param;
    window->i_frame = 0;
    
    x264_param_default_preset(param, "veryfast", "zerolatency");
    
    param->i_csp = X264_CSP_I420;
    param->i_width = width;
    param->i_height = height;
    param->i_slice_max_size = 149000;
    
    param->b_vfr_input = 0;
    param->b_repeat_headers = 1;
    param->b_annexb = 1;
    param->rc.f_rf_constant = 20;
    
    if (x264_param_apply_profile(param, "baseline" ) < 0) {
        printf("[Error] x264_param_apply_profile\n");
        return -1;
    }
    
    if (x264_picture_alloc(&window->pic, param->i_csp, param->i_width, param->i_height) < 0) {
        printf("[Error] x264_picture_alloc\n");
        return -1;
    }
    window->encoder = x264_encoder_open(param);
    if (!window->encoder){
        printf("[Error] x264_encoder_open\n");
        goto fail2;
    }
    
    return 0;
    
fail2:
    x264_picture_clean(&window->pic);
    return 1;
}

void cip_window_stream_reset(cip_window_t *window)
{
    uv_mutex_lock(&window->streamlock);
    x264_picture_clean(&window->pic);
    x264_encoder_close(window->encoder);
    cip_window_stream_init(window);
    uv_mutex_unlock(&window->streamlock);
}

void cip_window_frame_send(uv_async_t *handle)
{
    
    
    cip_window_t *window = handle->data;
    int force_keyframe = window->force_keyframe;
    
    if (window->width < 4 || window->height < 4) { /* too small */
        return;
    }
    uv_mutex_lock(&window->streamlock);
    int width = window->even_width;
    int height = window->even_height;
    x264_picture_t *pic = &window->pic;
    x264_picture_t picout;
    int i_frame_size;
    x264_nal_t *nal = NULL;
    int32_t i_nal = 0;
    
    xcb_get_image_reply_t *img;
    img = xcb_get_image_reply(cip_context.xconn,
                              xcb_get_image(cip_context.xconn, XCB_IMAGE_FORMAT_Z_PIXMAP, window->wid,
                                            0, 0, window->even_width, window->even_height, ~0 ), NULL);
    if ( img == NULL ) {// often coursed by GL window
        printf("get image error\n");
        uv_mutex_unlock(&window->streamlock);
        return;
    }
    //printf("length:%d width:%d,height:%d\n",xcb_get_image_data_length(img),width,height);
    
    
    ARGBToI420(xcb_get_image_data(img), width * 4,
               pic->img.plane[0], width,
               pic->img.plane[1], width / 2,
               pic->img.plane[2], width / 2,
               width, height );
    free(img);
    
    if (force_keyframe) {
        pic->i_type = X264_TYPE_KEYFRAME;
        window->force_keyframe = 0;
    }
    pic->i_pts = window->i_frame++;
    
    i_frame_size = x264_encoder_encode(window->encoder, &nal, &i_nal, &window->pic, &picout);
    if (i_frame_size < 0) {
        printf("[Error] x265_encoder_encode\n");
        uv_mutex_unlock(&window->streamlock);
        return;
    }
    //printf("i_frame_size:%d\n", i_frame_size);
    int i;
    for (i = 0; i < i_nal; ++i) {
        /* broadcast event */
        int length = sizeof(cip_event_window_frame_t) + nal[i].i_payload;
        char *buf = malloc(length);
        //printf("nal length:%d\n", length);
        cip_event_window_frame_t *p = (cip_event_window_frame_t*)buf;
        p->wid = window->wid;
        p->length = nal[i].i_payload;
        memcpy(buf + sizeof(cip_event_window_frame_t), nal[i].p_payload, nal[i].i_payload);
        
        
        /* add event to send list */
        write_req_t *wr = malloc(sizeof(write_req_t));
        wr->buf = uv_buf_init(buf, length);
        wr->channel_type = CIP_CHANNEL_DISPLAY;
        
        write_req_list_t *wr_list = async.data;
        list_head_t *event_list = &wr_list->requests;
        pthread_mutex_lock(&wr_list->mutex);
        list_add_tail(&wr->list_node, event_list);
        pthread_mutex_unlock(&wr_list->mutex);
        
        /* inform uv thread */
        uv_async_send(&async);
        
    }
    pic->i_type = X264_TYPE_AUTO;
    uv_mutex_unlock(&window->streamlock);
            
       
}
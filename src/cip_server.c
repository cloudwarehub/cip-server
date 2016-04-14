#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <uv.h>
#include "list.h"
#include "common.h"
#include "cip_protocol.h"
#include "cip_channel.h"
#include "cip_server.h"
#include "cip_window.h"
#include "cip_common.h"
#include "cip_session.h"

char *cip_session_test = "cloudwarehub";
cip_context_t cip_context;
    
uv_loop_t *loop;
uv_async_t async;

/* default cip port */
int port = 5999;
char display[20] = ":0";


static void alloc_buffer(uv_handle_t* handle,
                       size_t suggested_size,
                       uv_buf_t* buf)
{
    cip_channel_t *channel = (cip_channel_t*)handle->data;
    
    /* TODO: check ringbuf avaliable space */
    
    buf->base = (char*)ringbuf_tail(channel->rx_ring);
    buf->len = ringbuf_bytes_free(channel->rx_ring);
}

void after_write(uv_write_t *req, int status)
{
    write_req_t* wr;
    wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
    if (status != 0) {
        fprintf(stderr,
              "uv_write error: %s - %s\n",
              uv_err_name(status),
              uv_strerror(status));
    }
}

static void on_close(uv_handle_t* peer)
{
    
    free(peer);
    
}

static void after_shutdown(uv_shutdown_t* req, int status)
{
    uv_close((uv_handle_t*) req->handle, on_close);
    free(req);
    /* destroy channel ring buffer */
    cip_channel_t *cip_channel = req->handle->data;
    ringbuf_free(&cip_channel->rx_ring);
    free(cip_channel);
    
    /* free session */
    list_del(&cip_channel->session->list_node);
    free(cip_channel->session);
}


static void after_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
    printf("read bytes: %d\n", (int)nread);
    
    if (nread < 0) {
        /* Error or EOF */
        ASSERT(nread == UV_EOF);
        printf("colse channel\n");
        uv_shutdown_t* sreq = malloc(sizeof* sreq);
        ASSERT(0 == uv_shutdown(sreq, client, after_shutdown));
        return;
    }
    
    if (nread == 0) {
        /* Everything OK, but nothing read. */
        return;
    }
    
    /* get socket channel */
    cip_channel_t *channel = client->data;
    ringbuf_memcpy_into(channel->rx_ring, buf->base, nread);
    
    cip_channel_handle(channel);
}


void connection_cb(uv_stream_t *server, int status)
{
    printf("new conn\n");
    if (status != 0) {
        fprintf(stderr, "Connect error %s\n", uv_err_name(status));
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        /* create cip_channel_t and read connect msg from client */
        cip_channel_t *cip_channel = (cip_channel_t*)malloc(sizeof(cip_channel_t));
        cip_channel->connected = 0;
        cip_channel->rx_ring = ringbuf_new(1024);
        cip_channel->client = (uv_stream_t*)client;
        client->data = cip_channel;
        
        /* wait for connect msg, non block */
        uv_read_start((uv_stream_t*)client, alloc_buffer, after_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

void handle_property_change(xcb_property_notify_event_t *pne)
{
    switch (pne->atom) {
        case XCB_ATOM_WM_NAME:{
            xcb_icccm_get_text_property_reply_t data;
            xcb_icccm_get_wm_name_reply(cip_context.xconn, xcb_icccm_get_wm_name(cip_context.xconn, pne->window), &data, (void *)0);
            printf("%s\n", data.name);
            break;
        }
        default:
            break;
    }
}

void xorg_thread()
{
    xcb_connection_t *xconn;
    xcb_screen_t *screen;
    xcb_window_t root;
    list_head_t *event_list = async.data;
    
    xconn = cip_context.xconn;
    const xcb_query_extension_reply_t *query_damage_reply = xcb_get_extension_data(xconn, &xcb_damage_id);
    xcb_damage_query_version_reply(xconn, xcb_damage_query_version(xconn, 1, 1 ), 0);
    
    screen = xcb_setup_roots_iterator(xcb_get_setup(xconn)).data;
    root = screen->root;
    uint32_t mask[] = { XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_STRUCTURE_NOTIFY };
    xcb_change_window_attributes(xconn, root, XCB_CW_EVENT_MASK, mask);
    xcb_composite_redirect_subwindows(xconn, root, XCB_COMPOSITE_REDIRECT_AUTOMATIC);
    
    write_req_t *wr;
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event (xconn))) {
        printf("new event: %d\n", event->response_type);
        switch (event->response_type & ~0x80) {
            case XCB_CREATE_NOTIFY: {
                xcb_create_notify_event_t *e = (xcb_create_notify_event_t*)event;
                
                /* notify wm property changes */
                uint32_t mask[] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
                xcb_change_window_attributes_checked(xconn, e->window, XCB_CW_EVENT_MASK, mask);
                
                /* create window context and init stream context */
                cip_window_t *cip_window = malloc(sizeof(cip_window_t));
                INIT_LIST_HEAD(&cip_window->list_node);
                list_add(&cip_window->list_node, &cip_context.windows);
                cip_window->wid = e->window;
                cip_window->x = e->x;
                cip_window->y = e->y;
                cip_window->bare = e->override_redirect;
                cip_window->viewable = 0;
                cip_window->width = e->width;
                cip_window->height = e->height;
                cip_window_stream_init(cip_window);
                
                /* add event to send list */
                cip_event_window_create_t *cewc = malloc(sizeof(cip_event_window_create_t));
                cewc->type = CIP_EVENT_WINDOW_CREATE;
                cewc->wid = e->window;
                cewc->x = e->x;
                cewc->y = e->y;
                cewc->width = e->width;
                cewc->height = e->height;
                cewc->bare = e->override_redirect;
                wr = malloc(sizeof(write_req_t));
                wr->buf = uv_buf_init((char*)cewc, sizeof(*cewc));
                wr->channel_type = CIP_CHANNEL_EVENT;
                list_add_tail(&wr->list_node, event_list);
                
                /* inform uv thread to send event */
                uv_async_send(&async);
                break;
            }
            case XCB_DESTROY_NOTIFY:;
                xcb_destroy_notify_event_t *dne = (xcb_destroy_notify_event_t*)event;
                
                /* delete stream context */
                cip_window_t *cip_window;
                list_for_each_entry(cip_window, &cip_context.windows, list_node) {
                    if (cip_window->wid == dne->window) {
                        list_del(&cip_window->list_node);
                        free(cip_window);
                        break;
                    }
                }
                
                /* add event to send list */
                cip_event_window_destroy_t *cewd = malloc(sizeof(cip_event_window_destroy_t));
                cewd->type = CIP_EVENT_WINDOW_DESTROY;
                cewd->wid = dne->window;
                wr = malloc(sizeof(write_req_t));
                wr->buf = uv_buf_init((char*)cewd, sizeof(*cewd));
                wr->channel_type = CIP_CHANNEL_EVENT;
                list_add_tail(&wr->list_node, event_list);
                
                /* inform uv thread to send event */
                uv_async_send(&async);
                break;
            case XCB_MAP_NOTIFY:{
                xcb_map_notify_event_t *e = (xcb_map_notify_event_t*)event;
                
                /* set cip window viewable */
                cip_window_t *iter;
                list_for_each_entry(iter, &cip_context.windows, list_node) {
                    if (iter->wid == e->window) {
                        iter->viewable = 1;
                        break;
                    }
                }
                
                /* create damage */
                xcb_damage_damage_t damage = xcb_generate_id(xconn);
                xcb_damage_create(xconn, damage, e->window, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
                xcb_flush(xconn);
                
                /* add event to send list */
                cip_event_window_show_t *cews = malloc(sizeof(cip_event_window_show_t));
                cews->type = CIP_EVENT_WINDOW_SHOW;
                
                cews->wid = e->window;
                cews->bare = e->override_redirect;
                wr = malloc(sizeof(write_req_t));
                wr->buf = uv_buf_init((char*)cews, sizeof(*cews));
                wr->channel_type = CIP_CHANNEL_EVENT;
                list_add_tail(&wr->list_node, event_list);
                
                /* inform uv thread to send event */
                uv_async_send(&async);
                break;
            }
            case XCB_UNMAP_NOTIFY:;
                xcb_unmap_notify_event_t *umne = (xcb_unmap_notify_event_t*)event;
                
                /* add event to send list */
                cip_event_window_hide_t *cewh = malloc(sizeof(cip_event_window_hide_t));
                cewh->type = CIP_EVENT_WINDOW_HIDE;
                
                cewh->wid = umne->window;
                wr = malloc(sizeof(write_req_t));
                wr->buf = uv_buf_init((char*)cewh, sizeof(*cewh));
                wr->channel_type = CIP_CHANNEL_EVENT;
                list_add_tail(&wr->list_node, event_list);
                
                /* inform uv thread */
                uv_async_send(&async);
                break;
            case XCB_PROPERTY_NOTIFY: {
                xcb_property_notify_event_t *pne = (xcb_property_notify_event_t*)event;
                handle_property_change(pne);
                
                break;
            }
            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t *cone = (xcb_configure_notify_event_t*)event;
                //TODO: resize window stream
                
                /* add event to send list */
                cip_event_window_configure_t *cewc = malloc(sizeof(cip_event_window_configure_t));
                cewc->type = CIP_EVENT_WINDOW_CONFIGURE;
                cewc->wid = cone->window;
                cewc->x = cone->x;
                cewc->y = cone->y;
                cewc->width = cone->width;
                cewc->height = cone->height;
                cewc->bare = cone->override_redirect;
                cewc->above = cone->above_sibling;
                
                wr = malloc(sizeof(write_req_t));
                wr->buf = uv_buf_init((char*)cewc, sizeof(*cewc));
                wr->channel_type = CIP_CHANNEL_EVENT;
                list_add_tail(&wr->list_node, event_list);
                
                /* inform uv thread */
                uv_async_send(&async);
                break;
            }
            default:
                if (event->response_type == query_damage_reply->first_event + XCB_DAMAGE_NOTIFY) {
                    xcb_damage_notify_event_t *damage_event = (xcb_damage_notify_event_t*)event;
                    cip_window_frame_send(damage_event->drawable, 0);
                    xcb_damage_subtract(xconn, damage_event->damage, XCB_NONE, XCB_NONE);
                }
                break;
        }
    }
    printf("xorg thread end\n");
}

void emit_write(uv_async_t *handle)
{
    list_head_t *event_list = async.data;
    while (!list_empty(event_list)) {
        write_req_t *wr = list_entry(event_list->next, write_req_t, list_node);
        list_del(&wr->list_node);
        cip_session_t *sess;
        list_for_each_entry(sess, &cip_context.sessions, list_node) {
            /* copy write request for each session, if use same write request, libuv write callback will free wr many times */
            write_req_t *wr_cpy = malloc(sizeof(write_req_t));
            char *buf = malloc(wr->buf.len);
            memcpy(buf, wr->buf.base, wr->buf.len);
            wr_cpy->buf = uv_buf_init(buf, wr->buf.len);
            if (wr->channel_type == CIP_CHANNEL_EVENT && sess->event_channel) {
                printf("emit write event\n");
                uv_write(&wr_cpy->req, sess->event_channel->client, &wr_cpy->buf, 1, after_write);
            }
            if (wr->channel_type == CIP_CHANNEL_MASTER && sess->master_channel) {
                uv_write(&wr_cpy->req, sess->master_channel->client, &wr_cpy->buf, 1, after_write);
            }
            if (wr->channel_type == CIP_CHANNEL_DISPLAY && sess->display_channel) {
                uv_write(&wr_cpy->req, sess->display_channel->client, &wr_cpy->buf, 1, after_write);
            }
        }
        free(wr->buf.base);
        free(wr);
    }
}

void xorg_after(uv_work_t *req, int status)
{
    fprintf(stderr, "xorg end\n");
    free(async.data);
    uv_close((uv_handle_t*) &async, NULL);
}

void usage(char *exe)
{
    printf("Usage: %s -p [port(5999)] -d [display(:0)]\n", exe);
}


int main(int argc, char *argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "p:d:h")) != -1) {
        switch (ch) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'd':
                strcpy(display, optarg);
                break;
            case 'h':
            case '?':
                usage(argv[0]);
                exit(-1);
                break;
            default:
                break;
        }
    }
    INIT_LIST_HEAD(&cip_context.sessions);
    INIT_LIST_HEAD(&cip_context.windows);
    cip_context.xconn = xcb_connect(display, NULL);
    if (xcb_connection_has_error(cip_context.xconn)) {
        printf("cannot connect to xserver\n");
        xcb_disconnect(cip_context.xconn);
        return 1;
    }
    printf("connected to xorg\n");
    
    /* bellow is libuv routine */
    uv_tcp_t tcp_server;
    struct sockaddr_in addr;
    int r;
    
    loop = uv_default_loop();
    
    /* start xorg thread */
    uv_work_t req;
    
    /* async.data is a write request list */
    async.data = malloc(sizeof(list_head_t));
    INIT_LIST_HEAD((list_head_t*)async.data);
    list_head_t *write_list = malloc(sizeof(list_head_t));
    INIT_LIST_HEAD(write_list);
    req.data = write_list;
    uv_async_init(loop, &async, emit_write);
    uv_queue_work(loop, &req, xorg_thread, xorg_after);
    
    /* start tcp server loop */
    r = uv_ip4_addr("0.0.0.0", port, &addr);
    if (r) {
        fprintf(stderr, "convert addr error\n");
        return 1;
    }
    
    r = uv_tcp_init(loop, &tcp_server);
    if (r) {
        fprintf(stderr, "Socket creation error\n");
        return 1;
    }
    
    r = uv_tcp_bind(&tcp_server, (const struct sockaddr*)&addr, 0);
    if (r) {
        fprintf(stderr, "Bind error\n");
        return 1;
    }
    
    r = uv_listen((uv_stream_t*)&tcp_server, 128, connection_cb);
    if (r) {
        fprintf(stderr, "Listen error\n");
        return 1;
    }
    
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    printf("bye\n");
    return 0;
}
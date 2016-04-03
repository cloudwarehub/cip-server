#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xcb/xcb.h"
#include "xcb/composite.h"
#include "xcb/damage.h"
#include "uv.h"
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

int need_session = 0;

static void alloc_buffer(uv_handle_t* handle,
                       size_t suggested_size,
                       uv_buf_t* buf) {
    printf("alloc\n");
    cip_channel_t *channel = (cip_channel_t*)handle->data;
    
    /* if not connected, allocate cip_message_connect_t buf */
    if (!channel->connected) {
        buf->base = malloc(sizeof(cip_message_connect_t));
        buf->len = sizeof(cip_message_connect_t);
    } else {
        buf->base = malloc(suggested_size);
        buf->len = suggested_size;
    }
}

void after_write(uv_write_t *req, int status) {
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

static void on_close(uv_handle_t* peer) {
    free(peer);
}

static void after_shutdown(uv_shutdown_t* req, int status) {
    uv_close((uv_handle_t*) req->handle, on_close);
    free(req);
}

/* traverse cip sessions and find whose session match query */
cip_session_t *find_session(char *session)
{
    cip_session_t *iter = NULL;
    list_for_each_entry(iter, &cip_context.sessions, list_node) {
        if (strcmp(iter->session, session)) {
            return iter;
        }
    }
    return iter;
}

/* recover window state to event channel */
void recover_state(cip_channel_t *channel)
{
    // TODO
}
    
static void after_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    printf("read bytes: %d\n", (int)nread);

    if (nread < 0) {
        /* Error or EOF */
        ASSERT(nread == UV_EOF);
        
        free(buf->base);
        uv_shutdown_t* sreq = malloc(sizeof* sreq);
        ASSERT(0 == uv_shutdown(sreq, client, after_shutdown));
        return;
    }

    if (nread == 0) {
        /* Everything OK, but nothing read. */
        free(buf->base);
        return;
    }
    
    /* get socket channel */
    cip_channel_t *channel = (cip_channel_t*)client->data;
    write_req_t *wr;
    cip_message_connect_reply_t *msg_conn_rpl;
    if (!channel->connected) {
        /* TODO: check nread size */
        
        cip_message_connect_t *msg_conn = (cip_message_connect_t*)buf->base;
        switch (msg_conn->channel_type) {
            case CIP_CHANNEL_MASTER:
                printf("master channel\n");
                cip_check_version(msg_conn->version.major_version, msg_conn->version.minor_version);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_MASTER;
                channel->client = client;
                
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
                uv_write(&wr->req, client, &wr->buf, 1, after_write);
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
                    cip_session_dummy_t *sess_dummy = malloc(sizeof(cip_session_dummy_t));
                    sess_dummy->channel = channel;
                    list_add_tail(&sess_dummy->list_node, &cip_context.sessions);
                }
                wr->buf = uv_buf_init((char*)msg_conn_rpl, sizeof(cip_message_connect_reply_t));
                uv_write(&wr->req, client, &wr->buf, 1, after_write);
                
                channel->connected = 1;
                channel->type = CIP_CHANNEL_EVENT;
                channel->client = client;
                
                /* recover window state on channel established */
                recover_state(channel);
                break;
                
            default:
                break;
        }
        
    } else { /* channel already connected */
        /* TODO: check nread size */
        
        if (channel->type == CIP_CHANNEL_MASTER) {
            cip_message_t *msg = (cip_message_t*)buf->base;
        } else if (channel->type == CIP_CHANNEL_EVENT) {
            cip_event_t *m = (cip_event_t*)buf->base;
        }
    }
    
    free(buf->base);
}


void connection_cb(uv_stream_t *server, int status) {
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
        client->data = cip_channel;
        
        /* wait for connect msg, non block */
        uv_read_start((uv_stream_t*)client, alloc_buffer, after_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
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
                
                /* create window context and init stream context */
                cip_window_t *cip_window = malloc(sizeof(cip_window_t));
                INIT_LIST_HEAD(&cip_window->list_node);
                list_add(&cip_window->list_node, &cip_context.windows);
                cip_window->wid = e->window;
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
                
                /* inform uv thread send it */
                uv_async_send(&async);
                break;
            }
            case XCB_DESTROY_NOTIFY:;
                xcb_destroy_notify_event_t *dne = (xcb_destroy_notify_event_t*)event;
                cip_event_window_destroy_t cewd;
                cewd.type = CIP_EVENT_WINDOW_DESTROY;
                cewd.wid = dne->window;
                /* delete stream context */
                cip_window_t *cip_window;
                list_for_each_entry(cip_window, &cip_context.windows, list_node) {
                    if (cip_window->wid == dne->window) {
                        list_del(&cip_window->list_node);
                        free(cip_window);
                        break;
                    }
                }
                
//                list_for_each_entry(iter, &cip_context.sessions, list_node) {
//                    write_emit(iter->channel_event_sock, (char*)&cewd, sizeof(cewd));
//                }
                break;
            case XCB_MAP_NOTIFY:{
                xcb_map_notify_event_t *e = (xcb_map_notify_event_t*)event;
                usleep(500);
                
                // cip_window_t *wd;
                // list_for_each_entry(wd, &cip_context.windows, list_node) {
                //     if (wd->wid == e->window) {
                //         cip_window_stream_init(wd);
                //         break;
                //     }
                // }
                /* create damage */
                xcb_damage_damage_t damage = xcb_generate_id(xconn);
                xcb_damage_create(xconn, damage, e->window, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
                xcb_flush(xconn);
                
                cip_event_window_show_t cews;
                cews.type = CIP_EVENT_WINDOW_SHOW;
                cews.wid = e->window;
                cews.bare = e->override_redirect;
                
//                list_for_each_entry(iter, &cip_context.sessions, list_node) {
//                    write_emit(iter->channel_event_sock, (char*)&cews, sizeof(cews));
//                }
                
                break;
            }
            case XCB_UNMAP_NOTIFY:;
                xcb_unmap_notify_event_t *umne = (xcb_unmap_notify_event_t*)event;
                cip_event_window_hide_t cewh;
                cewh.type = CIP_EVENT_WINDOW_HIDE;
                cewh.wid = umne->window;
                
//                list_for_each_entry(iter, &cip_context.sessions, list_node) {
//                    write_emit(iter->channel_event_sock, (char*)&cewh, sizeof(cewh));
//                }
                break;
            default:
                if (event->response_type == query_damage_reply->first_event + XCB_DAMAGE_NOTIFY) {
                    xcb_damage_notify_event_t *damage_event = (xcb_damage_notify_event_t*)event;
                    cip_window_frame_send(damage_event->drawable);
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
            if (wr->channel_type == CIP_CHANNEL_EVENT && sess->event_channel) {
                uv_write(&wr->req, sess->event_channel->client, &wr->buf, 1, after_write);
            }
            if (wr->channel_type == CIP_CHANNEL_MASTER && sess->master_channel) {
                uv_write(&wr->req, sess->master_channel->client, &wr->buf, 1, after_write);
            }
            if (wr->channel_type == CIP_CHANNEL_DISPLAY && sess->display_channel) {
                uv_write(&wr->req, sess->display_channel->client, &wr->buf, 1, after_write);
            }
        }
    }
}

void xorg_after(uv_work_t *req, int status) {
    fprintf(stderr, "xorg end\n");
    free(async.data);
    uv_close((uv_handle_t*) &async, NULL);
}


int main()
{
    INIT_LIST_HEAD(&cip_context.sessions);
    INIT_LIST_HEAD(&cip_context.windows);
    cip_context.xconn = xcb_connect(NULL, NULL);
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
    r = uv_ip4_addr("0.0.0.0", 5999, &addr);
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
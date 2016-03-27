#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)

typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;
    
uv_loop_t *loop;



static void alloc_buffer(uv_handle_t* handle,
                       size_t suggested_size,
                       uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void after_write(uv_write_t *req, int status) {
    write_req_t* wr;
    wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
    if (status == 0)
    return;

    fprintf(stderr,
          "uv_write error: %s - %s\n",
          uv_err_name(status),
          uv_strerror(status));
}

static void on_close(uv_handle_t* peer) {
    free(peer);
}

static void after_shutdown(uv_shutdown_t* req, int status) {
    uv_close((uv_handle_t*) req->handle, on_close);
    free(req);
}
    
static void after_read(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
    write_req_t *wr;
    uv_shutdown_t* sreq;
    if (nread < 0) {
        /* Error or EOF */
        ASSERT(nread == UV_EOF);
        
        free(buf->base);
        sreq = malloc(sizeof* sreq);
        ASSERT(0 == uv_shutdown(sreq, handle, after_shutdown));
        return;
    }

    if (nread == 0) {
        /* Everything OK, but nothing read. */
        free(buf->base);
        return;
    }
    wr = (write_req_t*) malloc(sizeof *wr);
    wr->buf = uv_buf_init(buf->base, nread);
    uv_write(&wr->req, handle, &wr->buf, 1, after_write);
    
}


void connection_cb(uv_stream_t *server, int status) {
    printf("new conn\n");
    if (status == -1) {
        // error!
        return;
    }

    uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client);
    client->data = server;
    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_read_start((uv_stream_t*)client, alloc_buffer, after_read);
    }
    else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

int main()
{
    uv_tcp_t tcp_server;
    struct sockaddr_in addr;
    int r;
    
    loop = uv_default_loop();
    (r = uv_ip4_addr("127.0.0.1", 5996, &addr));
    printf("%d\n", r);
    // if (r) {
    //     fprintf(stderr, "error\n");
    //     return 1;
    // }
    
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
    
    uv_listen((uv_stream_t*)&tcp_server, 128, connection_cb);
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    printf("bye\n");
    return 0;
}
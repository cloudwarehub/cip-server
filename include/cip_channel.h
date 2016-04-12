#ifndef cip_channel_h
#define cip_channel_h

#include <uv.h>
#include "cip_protocol.h"
#include "cip_session.h"
#include "ringbuf.h"

typedef struct cip_session cip_session_t;

typedef struct cip_channel {
    int connected;
    enum CIP_CHANNEL type;
    uv_stream_t *client;
    cip_session_t *session;
    ringbuf_t rx_ring; /* used for receiving tcp chunk */
} cip_channel_t;

void cip_channel_handle(cip_channel_t *channel);

#endif
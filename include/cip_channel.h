#ifndef cip_channel_h
#define cip_channel_h

#include <uv.h>
#include "cip_protocol.h"
#include "ringbuf.h"

typedef struct {
    int connected;
    enum CIP_CHANNEL type;
    uv_stream_t *client;
    ringbuf_t rx_ring; /* used for receiving tcp chunk */
} cip_channel_t;

#endif
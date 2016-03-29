#ifndef cip_channel_h
#define cip_channel_h

#include <uv.h>
#include "cip_protocol.h"

typedef struct {
    int connected;
    enum CIP_CHANNEL type;
    uv_stream_t *client;
    char *tmp_buf; /* used for receiving tcp chunk */
} cip_channel_t;

#endif
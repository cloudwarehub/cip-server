#include "cip_protocol.h"

typedef struct {
    int connected;
    enum CIP_CHANNEL type;
    char *tmp_buf; /* used for receiving tcp chunk */
} cip_channel_t;
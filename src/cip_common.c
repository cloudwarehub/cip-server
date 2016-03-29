//
//  cip_common.c
//  cip-server
//
//  Created by gd on 16/3/29.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cip_protocol.h"
#include "cip_common.h"

int cip_check_version(uint8_t major, uint8_t minor)
{
    return 0;
}

/* generate random string */
char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (size) {
        --size;
        size_t n;
        for (n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

int event_len(cip_event_t *event)
{
    switch (event->type) {
        case CIP_EVENT_WINDOW_CREATE:
            return sizeof(cip_event_window_create_t);
            break;
        case CIP_EVENT_WINDOW_DESTROY:
            return sizeof(cip_event_window_destroy_t);
            break;
        case CIP_EVENT_WINDOW_SHOW:
            return sizeof(cip_event_window_show_t);
            break;
        case CIP_EVENT_WINDOW_HIDE:
            return sizeof(cip_event_window_hide_t);
            break;
        default:
            break;
    }
    return 0;
}
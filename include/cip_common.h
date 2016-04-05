//
//  cip_common.h
//  cip-server
//
//  Created by gd on 16/3/29.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#ifndef cip_common_h
#define cip_common_h

#include "cip_window.h"

int cip_check_version(uint8_t major, uint8_t minor);
char *rand_string(char *str, size_t size);
int event_len(cip_event_t *event);

int get_size_by_type(enum CIP_EVENT type);

cip_window_t *find_window(uint32_t wid, list_head_t *windows);

#endif /* cip_common_h */

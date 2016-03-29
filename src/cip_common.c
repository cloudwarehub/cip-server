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
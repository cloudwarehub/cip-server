//
//  cip_session.c
//  cip-server
//
//  Created by gd on 16/3/30.
//  Copyright © 2016年 CloudwareHub. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include "cip_session.h"
#include "cip_common.h"

void cip_session_init(cip_session_t *session)
{
    bzero(session, sizeof(cip_session_t));
    rand_string(session->session, sizeof(session->session));
}
/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
*   多线程异步网络框架，将逻辑层与网络层分离
*   两层之间通过消息队列通信
*/

#ifndef _ASYNNET_H
#define _ASYNNET_H

#include "netservice.h"
#include "thread.h"
#include "msgque.h"

typedef struct sock_ident{
	ident _ident;
}sock_ident;


#define TO_SOCK(IDENT) (*(sock_ident *)&IDENT)

typedef struct asynnet* asynnet_t;


asynnet_t  asynnet_new(uint8_t  pollercount);

void       asynnet_stop(asynnet_t);

void       asynnet_coronet(asynnet_t);

int32_t    asynsock_close(sock_ident);


int32_t    asyn_send(sock_ident,wpacket_t);

int32_t    get_addr_local(sock_ident,char *buf,uint32_t buflen);
int32_t    get_addr_remote(sock_ident,char *buf,uint32_t buflen);

int32_t    get_port_local(sock_ident,int32_t *port);
int32_t    get_port_remote(sock_ident,int32_t *port);


void       asynsock_set_ud(sock_ident,void*);
void*      asynsock_get_ud(sock_ident);


static inline ident rpk_read_ident(rpacket_t rpk)
{
    ident _ident;
    _ident.identity = rpk_read_uint64(rpk);
#ifdef _X64
        _ident.ptr = (void*)rpk_read_uint64(rpk);
#else
        _ident.ptr = (void*)rpk_read_uint32(rpk);
#endif
    return _ident;
}

static inline void wpk_write_ident(wpacket_t wpk,ident _ident)
{
    wpk_write_uint64(wpk,_ident.identity);
#ifdef _X64
        wpk_write_uint64(wpk,(uint64_t)_ident.ptr);
#else
        wpk_write_uint32(wpk,(uint32_t)_ident.ptr);
#endif
}

#endif

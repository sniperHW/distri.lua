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


#define CAST_2_SOCK(IDENT) (*(sock_ident*)&IDENT)

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


static inline int8_t eq_sockident(sock_ident a,sock_ident b)
{
    return a._ident.identity == b._ident.identity && a._ident.ptr == b._ident.ptr;
}

static inline sock_ident rpk_read_sock(rpacket_t rpk)
{
    sock_ident sock;
    sock._ident.identity = rpk_read_uint64(rpk);
    sock._ident.ptr = (void*)rpk_read_uint32(rpk);
    return sock;
}

static inline void wpk_write_sock(wpacket_t wpk,sock_ident sock)
{
    wpk_write_uint64(wpk,sock._ident.identity);
    wpk_write_uint32(wpk,(uint32_t)sock._ident.ptr);
}

#endif

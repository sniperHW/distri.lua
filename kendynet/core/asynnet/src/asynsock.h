#ifndef _ASYNSOCK_H
#define _ASYNSOCK_H
#include "connection.h"
#include "refbase.h"
#include "netservice.h"
#include "dlist.h"
#include "msgque.h"
#include "../asynnet.h"

typedef struct asynsock
{
	struct refbase          ref;
    struct dnode            dn;
	struct connection       *c;
	SOCK                     s;
    sock_ident              sident;
	msgque_t                sndque;//用于跟poller通信的消息队列
    msgque_t                que;   //用于跟应用层通信的消息队列
    void                    *ud;
}*asynsock_t;


asynsock_t asynsock_new(struct connection *c,SOCK s);

static inline asynsock_t cast_2_asynsock(sock_ident sock)
{
	ident _ident = TO_IDENT(sock);
    if(!is_type(_ident,type_asynsock))return NULL;
    struct refbase *r = cast_2_refbase(_ident);
    if(r) return (asynsock_t)r;
	return NULL;
}

static inline void asynsock_release(asynsock_t sock)
{
	ref_decrease((struct refbase*)sock);
}


struct msg_connection
{
    struct msg  base;
    char        ip[32];
    int32_t     port;
    uint32_t    reason;
};

struct msg_connect
{
    struct msg base;
    char       ip[32];
    int32_t    port;
    uint32_t   timeout;
};

struct msg_bind
{
    struct msg base;
    uint32_t send_timeout;
    uint32_t recv_timeout;
    int8_t raw;
};





#endif

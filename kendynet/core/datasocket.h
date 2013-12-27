#ifndef _DATASOCKET_H
#define _DATASOCKET_H
#include "Connection.h"
#include "refbase.h"
#include "netservice.h"
#include "double_link.h"
#include "msg_que.h"

typedef struct datasocket
{
	struct refbase          ref;
	struct double_link_node dn;
	union{
		struct connection       *c;
		SOCK                     s;
	};
	msgque_t                sndque;//用于跟poller通信的消息队列
}*datasocket_t;


datasocket_t create_datasocket(struct connection *c,SOCK s);

static inline datasocket_t cast_2_datasocket(struct ident sock)
{
	refbase *r = cast_2_refbase(sock);
	if(r) return (datasocket_t)r;
	return NULL;
}

static inline void release_datasocket(datasocket_t sock)
{
	ref_decrease((struct refbase*)sock);
}


#endif
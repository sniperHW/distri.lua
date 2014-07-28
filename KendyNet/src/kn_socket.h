#ifndef _KN_SOCKET_H
#define _KN_SOCKET_H

#include "kn_type.h"
#include "kn_list.h"

typedef struct{
	struct st_head comm_head;
	int    domain;
	int    type;
	int    protocal;
	int    inloop;	
	int    events;
	engine_t e;
	kn_list pending_send;//尚未处理的发请求
    kn_list pending_recv;//尚未处理的读请求
    struct kn_sockaddr    addr_local;
    struct kn_sockaddr    addr_remote;    
	void   (*cb_disconnect)(handle_t,int);
	void   (*cb_accept)(handle_t,void *ud);
	void   (*cb_ontranfnish)(handle_t,st_io*,int,int);
	void   (*cb_connect)(handle_t,int,void *ud);
	void   (*destry_stio)(st_io*);
}kn_socket;


#endif

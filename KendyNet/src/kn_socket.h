#ifndef _KN_SOCKET_H
#define _KN_SOCKET_H

#include "kn_type.h"
#include "kn_list.h"
#include "kendynet_private.h"
#include <assert.h>
#include <arpa/inet.h>
#include "kn_event.h"


//common struct of socket

typedef struct{
	handle comm_head;
	int    domain;
	int    type;
	engine_t e;
	kn_list pending_send;//尚未处理的发请求
    	kn_list pending_recv;//尚未处理的读请求
    	struct kn_sockaddr    addr_local;
    	struct kn_sockaddr    addr_remote;    
	void   (*callback)(handle_t,void*,int,int);
	void   (*clear_func)(void*);
}kn_socket;

enum{
	SOCKET_NONE = 0,
	SOCKET_ESTABLISH  = 1,
	SOCKET_CONNECTING = 2,
	SOCKET_LISTENING  = 3,
	SOCKET_CLOSE      = 4,
	SOCKET_DATAGRAM = 5,
};



#endif
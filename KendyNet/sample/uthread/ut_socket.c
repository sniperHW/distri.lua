#include "ut_socket.h"
#include "uthread/uhtead.h"
#include "kn_list.h"

static engine_t g_engine = NULL;

enum{
	UT_CONN,
	UT_SOCK,
};

typedef struct ut_socket{
	uint8_t type;
	union{
		stream_conn_t conn;
		handle_t            sock;
	};
	kn_list pending_accept;
	kn_list ut_block;
}*ut_socket_t;

struct st_node_accept{
	kn_list_node node;
	handle_t        sock;
};

struct st_node_block{
	kn_list_node node;
	uthread_t      _uthread;
};

int ut_socket_init(engine_t e){
	if(g_engine || !e) return -1;
	g_engine = e;
}

ut_socket_t ut_socket_new1(handle_t sock){
	ut_socket_t ut_socket = calloc(1,sizeof(*ut_socket));
	ut_socket->type = UT_SOCK;
	ut_socket->sock = sock;
	return ut_socket;
}

ut_socket_t ut_socket_new2(stream_conn_t conn){
	ut_socket_t ut_socket = calloc(1,sizeof(*ut_socket));
	ut_socket->type = UT_CONN;
	ut_socket->conn = conn;
	return ut_socket;
}

static void on_accept(handle_t s,void *ud){
	ut_socket_t _ut_sock = (ut_socket_t)ud;
	st_node_accept *node = calloc(1,sizeof(*node));
	node->sock = s;
	kn_list_pushback(&_ut_sock->pending_accept,(kn_list_node*)node);
	//if there is uthread block on accept,wake it up;
	struct st_node_block *_block = (struct st_node_block*)kn_list_pop(&_ut_sock->ut_block);
	if(_block){
		ut_wakeup(_block->_uthread);
		free(_block);
	}
}

ut_socket_t ut_socket_listen(kn_sockaddr *local){
	if(!g_engine) return NULL;
	handle_t sock = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(!sock) return NULL;
	ut_socket_t _ut_sock = ut_socket_new1(sock);
	kn_list_init(&_ut_sock.pending_accept);
	kn_sock_listen(g_engine,sock,&local,on_accept,(void*)_ut_sock);
	return _ut_sock;
}

ut_socket_t ut_accept(ut_socket_t _utsock){
	uthread_t  ut_current = ut_getcurrent();
	if(is_empty_ident(ut_current)) return NULL;
	ut_socket_t _utsock = NULL;
	while(1){
		struct st_node_accept *_node = (struct st_node_accept*)kn_list_pop(&_ut_sock->pending_accept);
		if(_node){			

			free(_node);
			break;
		}
		ut_block(ut_current);
	}
	return _utsock;
}

ut_socket_t ut_connect(kn_sockaddr *remote);
packet_t      ut_recv(ut_socket_t);
int                ut_send(ut_socket_t);
int                ut_close(ut_socket_t);
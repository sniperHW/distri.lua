#include "kn_stream_conn_server.h"
#include "kn_stream_conn_private.h"
#include "kn_proactor.h"

typedef struct kn_stream_server{
	struct service base;
	kn_proactor_t proactor;
	void (*on_connection)(struct kn_stream_server*,kn_stream_conn_t);
	kn_fd_t      listen_fd;
	kn_sockaddr  server_addr;
	kn_timer_t   timer;
}kn_stream_server,*kn_stream_server_t;

static void on_new_connection(kn_fd_t fd,void *ud){
	
	kn_stream_server_t server = (kn_stream_server_t)ud;
	assert(server->on_connection);
	server->on_connection(server,kn_new_stream_conn(fd));
}

static void tick(struct service *s){
	//检查连接超时
}


kn_stream_server_t kn_new_stream_server(kn_proactor_t p,
									    kn_sockaddr *serveraddr,
									    void (*on_connect)(struct kn_stream_server*,kn_stream_conn_t))
{
	kn_fd_t l;
	kn_stream_server_t server = calloc(1,sizeof(*server));
	if(serveraddr){
		l = kn_listen(p,serveraddr,on_new_connection,server);
		if(!l){
			free(server);
			return NULL;
		}
		server->listen_fd = l;
		server->server_addr = *serveraddr;
	}
	
	server->on_connection = on_connect;
	server->timer = kn_new_timer();
	server->proactor = p;
	server->base.tick = tick;
	kn_dlist_push(&p->service,(kn_dlist_node*)server);
	return server;
}

void kn_destroy_stream_server(kn_stream_server_t server){
	if(server->listen_fd) kn_closefd(server->listen_fd);
	kn_delete_timer(server->timer);
	kn_dlist_remove((kn_dlist_node*)&server);
	free(server);
}

void IoFinish(kn_fd_t fd,st_io *io,int32_t bytestransfer,int32_t err_code);
void start_recv(kn_stream_conn_t c);

int kn_stream_server_bind(kn_stream_server_t server,
						   kn_stream_conn_t conn,
						   int8_t   is_raw, 
						   uint32_t recvbuf_size,
					       int(*on_packet)(kn_stream_conn_t,rpacket_t),//on packet
					       void (*on_disconnected)(kn_stream_conn_t,int err),//on disconnect
						   uint32_t recv_timeout,
						   void (*on_recv_timeout)(kn_stream_conn_t),//on recv timeout
						   uint32_t send_timeout,
						   void (*on_send_timeout)(kn_stream_conn_t) //on send timeout									
						   )
{
	
	if(recvbuf_size == 0) recvbuf_size = 1024;
	conn->raw = is_raw;
	conn->recv_bufsize = recvbuf_size; 
	conn->recv_timeout = recv_timeout;
	conn->send_timeout = send_timeout;
	conn->on_recv_timeout = on_recv_timeout;
	conn->on_send_timeout = on_send_timeout;
	conn->on_packet = on_packet;
	if(0 == kn_proactor_bind(server->proactor,conn->fd,IoFinish)){
		start_recv(conn);
		return 0;
	}else
		return -1;
}	

#include "kn_stream_conn_client.h"
#include "kn_stream_conn_private.h"
#include "kn_proactor.h"

typedef struct kn_stream_client{
	struct service base;
	kn_proactor_t proactor;
	void (*on_connection)(kn_stream_client_t,kn_stream_conn_t);
	void (*on_connect_fail)(kn_stream_client_t,kn_sockaddr *addr,int err);
	kn_timer_t   timer;
}kn_stream_server,*kn_stream_server_t;

static void connect_cb(kn_fd_t fd,struct kn_sockaddr* addr,void *ud,int err)
{
	kn_stream_client_t client = (kn_stream_client_t)ud;
	if(fd){
		assert(client->on_connection);
		client->on_connection(client,kn_new_stream_conn(fd));
	}else{
		client->on_connect_fail(client,addr,err);
	} 	
}

void kn_stream_client_tick(struct service *s);
kn_stream_client_t kn_new_stream_client(kn_proactor_t p,
										void (*on_connect)(kn_stream_client_t,kn_stream_conn_t),
										void (*on_connect_fail)(kn_stream_client_t,kn_sockaddr*,int err))
{
	kn_stream_client_t client = calloc(1,sizeof(*client));

	client->on_connection = on_connect;
	client->on_connect_fail = on_connect_fail;
	client->timer = kn_new_timer();
	client->proactor = p;
	client->base.tick = NULL;
	kn_dlist_init(&client->base.dlist);
	if(client->base.tick)
		kn_dlist_push(&p->service,(kn_dlist_node*)client);
	return client;
}

void kn_destroy_stream_client(kn_stream_client_t client){
	kn_delete_timer(client->timer);
	kn_dlist_remove((kn_dlist_node*)&client);
	kn_dlist_node *node;
	while((node = kn_dlist_pop(&client->base.dlist))){
		kn_stream_conn_t conn = (kn_stream_conn_t)node;
		if(conn->on_disconnected) conn->on_disconnected(conn,0);
		kn_closefd(conn->fd);
	}		
	free(client);
}

int bind_private(kn_proactor_t p,
			     kn_stream_conn_t conn,
			     int8_t   is_raw, 
			     uint32_t recvbuf_size,
			     int(*on_packet)(kn_stream_conn_t,rpacket_t),//on packet
			     void (*on_disconnected)(kn_stream_conn_t,int err),//on disconnect
			     uint32_t recv_timeout,
			     void (*on_recv_timeout)(kn_stream_conn_t),//on recv timeout
			     uint32_t send_timeout,
			     void (*on_send_timeout)(kn_stream_conn_t) //on send timeout									
			     );

int kn_stream_client_bind( kn_stream_client_t client,
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
	int ret = bind_private(client->proactor,conn,is_raw,recvbuf_size,
						on_packet,on_disconnected,recv_timeout,
						on_recv_timeout,send_timeout,on_send_timeout);
	
	if(ret == 0){
		kn_dlist_push(&client->base.dlist,(kn_dlist_node*)conn);
		conn->service = (struct service*)client;
	}
	return ret;
}

int kn_stream_connect(kn_stream_client_t client,
					  struct kn_sockaddr *addr_local,				  
			          struct kn_sockaddr *addr_remote,
			          uint64_t timeout)
{
	return kn_asyn_connect(client->proactor,SOCK_STREAM,addr_local,addr_remote,
						   connect_cb,(void*)client,timeout);	
}	

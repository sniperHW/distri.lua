#include "kn_stream_conn_client.h"
#include "kn_stream_conn_private.h"
#include "kn_proactor.h"

typedef struct kn_stream_client{
	kn_proactor_t proactor;
	kn_dlist      dlist;
	void (*on_connection)(kn_stream_client_t,kn_stream_conn_t,void *);
	void (*on_connect_fail)(kn_stream_client_t,kn_sockaddr *addr,int err,void *);
}kn_stream_client,*kn_stream_client_t;

struct connect_context{
	kn_stream_client_t client;
	void*              ud;
};

static void connect_cb(kn_fd_t fd,struct kn_sockaddr* addr,void *ud,int err)
{
	struct connect_context *c = ud;
	kn_stream_client_t client = c->client;
	if(fd){
		assert(client->on_connection);
		client->on_connection(client,kn_new_stream_conn(fd),c->ud);
	}else{
		client->on_connect_fail(client,addr,err,c->ud);
	}
	free(c);
}

kn_stream_client_t kn_new_stream_client(kn_proactor_t p,
										void (*on_connect)(kn_stream_client_t,kn_stream_conn_t,void *ud),
										void (*on_connect_fail)(kn_stream_client_t,kn_sockaddr*,int err,void *ud))
{
	kn_stream_client_t client = calloc(1,sizeof(*client));

	client->on_connection = on_connect;
	client->on_connect_fail = on_connect_fail;
	client->proactor = p;
	kn_dlist_init(&client->dlist);
	return client;
}

void kn_destroy_stream_client(kn_stream_client_t client){
	kn_dlist_remove((kn_dlist_node*)&client);
	kn_dlist_node *node;
	while((node = kn_dlist_pop(&client->dlist))){
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
		kn_dlist_push(&client->dlist,(kn_dlist_node*)conn);
	}
	return ret;
}

int kn_stream_connect(kn_stream_client_t client,
		struct kn_sockaddr *addr_local,				  
		struct kn_sockaddr *addr_remote,
		void   *ud)
{

	struct connect_context *c = calloc(1,sizeof(*c));
	c->client = client;
	c->ud = ud;
	return kn_asyn_connect(client->proactor,SOCK_STREAM,addr_local,addr_remote,
						   connect_cb,(void*)c);	
}	

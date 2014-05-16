#include <stdio.h>
#include "kendynet.h"
#include "kn_stream_conn_client.h"

static uint32_t packet_size;
static kn_stream_client_t c;
static kn_sockaddr remote;

static int  on_packet(kn_stream_conn_t conn,rpacket_t rpk){
	//kn_stream_conn_send(conn,wpk_create_by_rpacket(rpk));
	return 1;
}

static void on_disconnected(kn_stream_conn_t conn,int err){
	printf("on_disconnected\n");
	//kn_stream_connect(c,NULL,&remote,3*1000);
}

static void on_connected(kn_stream_client_t client,kn_stream_conn_t conn){
	printf("on_connected\n");
	kn_stream_client_bind(client,conn,0,1024,on_packet,on_disconnected,
						  0,NULL,0,NULL);
	//wpacket_t wpk = NEW_WPK(64);
	//wpk_write_string(wpk,"hello kenny");
	//kn_stream_conn_send(conn,wpk);							  
}

static void on_connect_failed(kn_stream_client_t client,kn_sockaddr *addr,int err)
{
		printf("connect_fail\n");
}

int main(int argc,char **argv)
{
	kn_proactor_t p = kn_new_proactor();
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));		
	c = kn_new_stream_client(p,on_connected,on_connect_failed);
	int client_count = atoi(argv[3]);
	packet_size = atoi(argv[4]);
	
	int i = 0;
	for(; i < client_count; ++i){
		kn_stream_connect(c,NULL,&remote,3*1000);
	}
	
	while(1){
		kn_proactor_run(p,50);
	}
	return 0;
}

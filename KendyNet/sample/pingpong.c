#include "kendynet.h"
#include "stream_conn.h"
#include "kn_timer.h"

int  client_count = 0; 

void  on_packet(stream_conn_t c,packet_t p){
	
	rpacket_t rpk = (rpacket_t)p;
	const char *str = rpk_read_string(rpk);
	if(strcmp(str,"hello") != 0)
	{
		printf("error\n");
		exit(0);
	}
	stream_conn_send(c,(packet_t)wpk_copy_create(p));
}

void on_disconnected(stream_conn_t c,int err){
	printf("on_disconnectd\n");
}

void on_connect(handle_t s,int err,void *ud,kn_sockaddr *_)
{
	((void)_);
	if(s && err == 0){
		printf("connect ok\n");
		engine_t p = (engine_t)ud;
		stream_conn_t conn = new_stream_conn(s,4096,new_rpk_decoder(4096));
		stream_conn_associate(p,conn,on_packet,on_disconnected);		
		wpacket_t wpk = wpk_create(64);
		wpk_write_string(wpk,"hello");		
		stream_conn_send(conn,(packet_t)wpk);		
	}else{
		printf("connect failed\n");
	}	
}


int timer_callback(kn_timer_t timer){
	printf("client_count:%d\n",client_count);
	return 1;
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int i = 0;
	for(; i < client_count; ++i){
		handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		kn_sock_connect(p,c,&remote,NULL,on_connect,p);
	}
	
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}

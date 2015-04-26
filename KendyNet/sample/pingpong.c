#include "kendynet.h"
#include "connection.h"
#include "kn_timer.h"

int  client_count = 0; 

void  on_packet(connection_t c,packet_t p){
	
	rpacket_t rpk = (rpacket_t)p;
	const char *str = rpk_read_string(rpk);
	if(strcmp(str,"hello") != 0)
	{
		printf("error\n");
		exit(0);
	}
	connection_send(c,(packet_t)make_writepacket(p),NULL);
}

void on_disconnected(connection_t c,int err){
	printf("on_disconnectd\n");
}

void on_connect(handle_t s,void *_1,int _2,int err)
{
	if(s && err == 0){
		printf("connect ok\n");
		engine_t p = kn_sock_engine(s);
		connection_t conn = new_connection(s,4096,new_rpk_decoder(4096));
		connection_associate(p,conn,on_packet,on_disconnected);		
		wpacket_t wpk = wpk_create(64);
		wpk_write_string(wpk,"hello");		
		connection_send(conn,(packet_t)wpk,NULL);		
	}else{
		printf("connect failed\n");
	}	
}


int timer_callback(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){
		printf("client_count:%d\n",client_count);
	}
	return 0;
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
		int ret = kn_sock_connect(c,&remote,NULL);
		if(ret > 0){
			connection_t conn = new_connection(c,4096,new_rpk_decoder(4096));
			connection_associate(p,conn,on_packet,on_disconnected);		
			wpacket_t wpk = wpk_create(64);
			wpk_write_string(wpk,"hello");		
			connection_send(conn,(packet_t)wpk,NULL);	
		}else if(ret == 0){
			kn_engine_associate(p,c,on_connect);
		}else{
			kn_close_sock(c);
			printf("connect failed\n");
		}		
	}
	
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}

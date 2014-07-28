#include "kendynet.h"
#include "sesion.h"
#include <stdio.h>

int send_size;

void on_connect(handle_t s,int err,void *ud)
{
	engine_t p = (engine_t)ud;
	if(s){
		printf("connect ok\n");
		struct session *session = calloc(1,sizeof(*session));
		session->s = s;
		kn_sock_setud(s,session);    	
		session_send(session,send_size);
	}else{
		printf("connect failed\n");
	}	
}

int main(int argc,char **argv){
	engine_t p = kn_new_engine();
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int i = 0;
	send_size = atoi(argv[4]);
	for(; i < client_count; ++i){
		handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		kn_sock_associate(s,p,transfer_finish,NULL);
		kn_sock_connect(c,&remote,NULL,on_connect,p);
	}
	
	kn_engine_run(p);
	return 0;
}

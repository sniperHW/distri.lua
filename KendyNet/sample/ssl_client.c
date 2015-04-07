#include "kendynet.h"
#include <stdio.h>
#include "session.h"



void on_connect(handle_t s,int err,void *ud,kn_sockaddr *_)
{
	((void)_);
	if(err == 0){
		printf("connect ok\n");
		struct session *session = calloc(1,sizeof(*session));
		session->s = s;
		engine_t p = (engine_t)ud;
		kn_sock_associate(p,s,transfer_finish,NULL);	
		kn_sock_setud(s,session);    	
		session_send(session,4096);
	}else{
		printf("connect failed\n");
	}	
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	SSL_init();
	engine_t p = kn_new_engine();
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int i = 0;
	for(; i < client_count; ++i){
		handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		int ret = kn_sock_sslconnect(p,c,&remote,NULL);
		if(ret > 0){
			on_connect(c,0,p,&remote);
		}else if(ret == 0){
			kn_sock_set_connect_cb(c,on_connect,p);
		}else{
			kn_close_sock(c);
			printf("connect failed\n");
		}
	}
	kn_engine_run(p);
	return 0;
}

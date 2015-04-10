#include "kendynet.h"
#include "session.h"
#include <stdio.h>

int send_size;

void on_connect(handle_t s,void *_1,int _2,int err)
{
	if(s && err == 0){
		printf("connect ok\n");
		struct session *session = calloc(1,sizeof(*session));
		session->s = s;
		engine_t p = kn_sock_engine(s);
		kn_engine_associate(p,s,transfer_finish);	
		kn_sock_setud(s,session);    	
		session_send(session,send_size);
	}else{
		printf("connect failed\n");
	}	
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int i = 0;
	send_size = atoi(argv[4]);
	for(; i < client_count; ++i){
		handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		int ret = kn_sock_connect(c,&remote,NULL);
		if(ret > 0){
			//on_connect(c,&remote,0,0);
			//on_connect(c,0,p,&remote);
			struct session *session = calloc(1,sizeof(*session));
			session->s = c;
			kn_engine_associate(p,c,transfer_finish);	
			kn_sock_setud(c,session);    	
			session_send(session,send_size);			
		}else if(ret == 0){
			kn_engine_associate(p,c,on_connect);
			//kn_sock_set_connect_cb(c,on_connect,p);
		}else{
			kn_close_sock(c);
			printf("connect failed\n");
		}
	}
	
	kn_engine_run(p);
	return 0;
}

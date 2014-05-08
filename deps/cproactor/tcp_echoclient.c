#include <stdio.h>
#include "kendynet.h"
#include "session.h"

int send_size;

void on_connect(kn_fd_t s,struct kn_sockaddr *remote,void *ud,int err)
{
	kn_proactor_t p = (kn_proactor_t)ud;
	if(s){
		printf("connect ok\n");
		struct session *session = calloc(1,sizeof(*session));
		session->s = s;
		kn_fd_setud(s,session);    	
		kn_proactor_bind(p,s,transfer_finish);
		session_send(session,send_size);
	}else{
		printf("connect failed\n");
	}	
}

int main(int argc,char **argv){
	kn_net_open();	
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr remote;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int i = 0;
	send_size = atoi(argv[4]);
	for(; i < client_count; ++i){
		kn_asyn_connect(p,SOCK_STREAM,NULL,&remote,on_connect,(void*)p,30000);
	} 
	while(1){
		kn_proactor_run(p,50);
	}
	return 0;
}






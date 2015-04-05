#include "kendynet.h"
#include "session.h"
#include "kn_timer.h"

void on_accept(handle_t s,void *ud){
	printf("on_accept\n");
	engine_t p = (engine_t)ud;
	struct session *session = calloc(1,sizeof(*session));
	session->s = s;
	kn_sock_setud(s,session);
	kn_engine_associate(p,s,transfer_finish,NULL);	
	session_recv(session);
	++client_count;
	printf("%d\n",client_count);   
}

int timer_callback(kn_timer_t timer){
	printf("client_count:%d,totalbytes:%ld MB/s\n",client_count,totalbytes/1024/1024);
	totalbytes = 0;
	return 1;
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sock_listen(p,l,&local,on_accept,p);
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}

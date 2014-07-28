#include "kendynet.h"
#include "sesion.h"

void on_accept(handle_t s,void *ud){
	printf("on_accept\n");
	engine_t p = (engine_t)ud;
	struct session *session = calloc(1,sizeof(*session));
	session->s = s;
	kn_sock_setud(s,session);
	kn_sock_associate(s,p,transfer_finish,NULL);	
	session_recv(session);
	++client_count;
	printf("%d\n",client_count);   
}

int timer_callback(kn_timer_t timer){
	struct userst *st = (struct userst*)kn_timer_getud(timer);
	printf("%d,%d\n",kn_systemms64()-st->expecttime,st->ms);
	st->expecttime = kn_systemms64() + st->ms;
	return 1;
}

int main(int argc,char **argv){
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sock_associate(l,p,NULL,NULL);
	kn_sock_listen(l,&local,on_accept,p);
	
	struct userst st1;
	st1.ms = 1005;
	st1.expecttime = kn_systemms64()+st2.ms;
	kn_reg_timer(p,st1.ms,timer_callback,&st1);		
	kn_engine_run(p);
	return 0;
}

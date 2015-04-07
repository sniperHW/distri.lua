#include "kendynet.h"
#include "session.h"
#include "kn_timer.h"
#include "kn_daemonize.h"

void on_accept(handle_t s,void *ud){
	engine_t p = (engine_t)ud;
	struct session *session = calloc(1,sizeof(*session));
	session->s = s;
	kn_sock_setud(s,session);
	kn_sock_associate(p,s,transfer_finish,NULL);	
	session_recv(session);
	++client_count;
}

int timer_callback(kn_timer_t timer){
	totalbytes = 0;
	return 1;
}

int main(int argc,char **argv){	
	set_lockfile("/var/run/daemonserver.pid");
	if(0 != already_running()){
		printf("already_running\n");
		return 0;
	}
	daemonize("daemonserver");
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sock_listen(p,l,&local,on_accept,p);
	kn_engine_run(p);
	return 0;
}

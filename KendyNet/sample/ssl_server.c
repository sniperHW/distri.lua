#include "kendynet.h"
#include "session.h"
#include "kn_time.h"
#include "kn_timer.h"

void on_accept(handle_t s,void *listener,int _2,int _3){
	engine_t p = kn_sock_engine((handle_t)listener);
	struct session *session = calloc(1,sizeof(*session));
	session->s = s;
	kn_sock_setud(s,session);
	kn_engine_associate(p,s,transfer_finish);	
	session_recv(session);
	++client_count;
	printf("%d\n",client_count); 
}

int timer_callback(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){
		printf("client_count:%d,totalbytes:%ld MB/s\n",client_count,totalbytes/1024/1024);
		totalbytes = 0;
	}
	return 0;
}


int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	SSL_init();
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if( 0 == kn_sock_ssllisten(l,&local,"cacert.pem","privkey.pem")){
		kn_engine_associate(p,l,on_accept);
	}else{
		printf("listen error\n");
		return 0;
	}	
	kn_reg_timer(p,1000,timer_callback,NULL);				
	kn_engine_run(p);
	return 0;
}
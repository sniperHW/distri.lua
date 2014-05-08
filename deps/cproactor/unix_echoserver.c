#include <stdio.h>
#include "kendynet.h"
#include "kn_time.h"
#include "session.h"


void on_accept(kn_fd_t s,void *ud){
	printf("on_accept\n");
	kn_proactor_t p = (kn_proactor_t)ud;
	struct session *session = calloc(1,sizeof(*session));
	session->s = s;
	kn_fd_setud(s,session);    	
	kn_proactor_bind(p,s,transfer_finish);
	session_recv(session);
	++client_count;
}


int main(int argc,char **argv){
	kn_net_open();	
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr local;
	unlink(argv[1]);
	kn_addr_init_un(&local,argv[1]);
	kn_listen(p,&local,on_accept,(void*)p);
 	uint64_t tick,now;
    tick = now = kn_systemms64();	
	while(1){
		kn_proactor_run(p,50);
        now = kn_systemms64();
		if(now - tick > 1000)
		{
            uint32_t elapse = (uint32_t)(now-tick);
            totalbytes = (totalbytes/elapse)/1024;
			printf("client_count:%d,totalbytes:%lldMB/s\n",client_count,totalbytes);
			tick = now;
			totalbytes = 0;
		}		
	}
	return 0;
}






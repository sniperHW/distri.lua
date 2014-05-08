#include "kendynet.h"
#include "kn_time.h"

#include "dgram_session.h"

int main(int argc,char **argv){
	kn_net_open();	
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr local;
	kn_fd_t l;
	unlink(argv[1]);
	kn_addr_init_un(&local,argv[1]);
	//kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	l = kn_dgram_listen(p,SOCK_DGRAM,&local,transfer_finish);	
 	struct dgram_session *session = calloc(1,sizeof(*session));
	session->s = l;  
 	kn_fd_setud(l,session);
 	dgram_session_recv(session);    		
 	uint64_t tick,now;
    tick = now = kn_systemms64();	
	while(1){
		kn_proactor_run(p,50);
        now = kn_systemms64();
		if(now - tick > 1000)
		{
            uint32_t elapse = (uint32_t)(now-tick);
            totalbytes = (totalbytes/elapse)/1024;
			printf("totalbytes:%lldMB/s\n",totalbytes);
			tick = now;
			totalbytes = 0;
		}		
	}
	return 0;
}


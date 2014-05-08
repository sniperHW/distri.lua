#include "kendynet.h"
#include "kn_time.h"

#include "dgram_session.h"



int main(int argc,char **argv){
	kn_net_open();	
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr remote;
	kn_fd_t c;
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	int client_count = atoi(argv[3]);
	int send_size = atoi(argv[4]);
	int i = 0;
	for(; i < client_count; ++i){
		c = kn_connect(SOCK_DGRAM,NULL,&remote);
		struct dgram_session *session = calloc(1,sizeof(*session));
		session->s = c;  
		kn_fd_setud(c,session);
		kn_proactor_bind(p,c,transfer_finish);
		dgram_session_send(session,send_size,&remote);
	} 
	while(1){
		kn_proactor_run(p,50);
	}
	return 0;
}

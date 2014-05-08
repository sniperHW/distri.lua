#include "kendynet.h"
#include "kn_time.h"

#include "dgram_session.h"



int main(int argc,char **argv){
	kn_net_open();	
	kn_proactor_t p = kn_new_proactor();
	kn_sockaddr remote,local;
	kn_fd_t c;
	char path[128];
	kn_addr_init_un(&remote,argv[1]);	
	int client_count = atoi(argv[3]);
	int send_size = atoi(argv[4]);
	int i = 0;
	for(; i < client_count; ++i){
		
		snprintf(path,128,"%s%i",argv[2],i);
		unlink(path);
		kn_addr_init_un(&local,path);
		c = kn_connect(SOCK_DGRAM,&local,&remote);
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

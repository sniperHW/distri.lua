#include "kendynet.h"
#include "kn_timer.h"
#include "datagram.h"

double totalbytes   = 0; 

void on_packet(struct datagram *d,packet_t rpk,kn_sockaddr *from){
	totalbytes += packet_datasize(rpk);
	datagram_send(d,(packet_t)make_writepacket(rpk),from);
}

int timer_callback(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){
		printf("totalbytes:%f MB/s\n",totalbytes/1024/1024);
		totalbytes = 0;
	}
	return 0;
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	datagram_t d = datagram_listen(&local,2048,NULL);
	datagram_associate(p,d,on_packet);
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}
#include "kendynet.h"
#include "datagram.h"
#include "time.h"

void on_packet(struct datagram *d,packet_t rpk,kn_sockaddr *from){
	datagram_send(d,(packet_t)make_writepacket(rpk),from);
}

int main(int argc,char **argv){
	char buf[2048];
	const char *ip = argv[1];
	int port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();	
	datagram_t d = new_datagram(AF_INET,2048,NULL);
	datagram_associate(p,d,on_packet);
	kn_sockaddr to;
	kn_addr_init_in(&to,ip,port);
	datagram_send(d,(packet_t)rawpacket_create2(buf,2048),&to);
	kn_engine_run(p);
	return 0;
}
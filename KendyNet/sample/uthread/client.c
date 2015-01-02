#include "ut_socket.h"
#include "uthread/uthread.h"
#include <stdio.h>
#include <stdlib.h>

kn_sockaddr remote;

void uthread_worker(void *arg){
	ut_socket_t sock = ut_connect(&remote,4096,new_rpk_decoder(4096));
	if(!is_empty_ident(sock)){
		wpacket_t wpk = wpk_create(64);
		wpk_write_uint64(wpk,100);
		ut_send(sock,(packet_t)wpk);
		packet_t packet = NULL;
		int err = 0;
		while(ut_recv(sock,&packet,&err) == 0){
			packet_t wpk = make_writepacket(packet);
			destroy_packet(packet);
			ut_send(sock,wpk);
		}		
	}else{
		printf("connect error\n");
	}
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	kn_addr_init_in(&remote,argv[1],atoi(argv[2]));
	engine_t p = kn_new_engine();
	ut_socket_init(p);
	uscheduler_init(8192);
	int uthread_count = atoi(argv[3]);
	int i =0;
	for(; i < uthread_count; ++i){
		ut_spawn(uthread_worker,NULL);
	}
	ut_socket_run();
	return 0;
}
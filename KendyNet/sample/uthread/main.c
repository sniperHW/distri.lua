#include "ut_socket.h"
#include "uthread/uthread.h"
#include <stdio.h>
#include <stdlib.h>

kn_sockaddr local;

void uthread_worker(void *arg){
	ut_socket_t client = *(ut_socket_t*)arg;
	free(arg);
	packet_t packet = NULL;
	int err = 0;
	while(ut_recv(client,&packet,&err) == 0){
		packet_t wpk = make_writepacket(packet);
		destroy_packet(packet);
		ut_send(client,wpk);
	}
}


void uthread_accept(void *arg){
	printf("uthread_accept\n");
	ut_socket_t server = *(ut_socket_t*)arg;
	while(1){
		ut_socket_t *client = calloc(1,sizeof(*client));
		*client = ut_accept(server,4096,NULL);
		printf("new client\n");
		ut_spawn(uthread_worker,(void*)client);
	}
}


int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	engine_t p = kn_new_engine();
	ut_socket_init(p);
	uscheduler_init(4096);
	ut_socket_t server = ut_socket_listen(&local);
	ut_spawn(uthread_accept,(void*)&server);
	ut_socket_run();
	return 0;
}
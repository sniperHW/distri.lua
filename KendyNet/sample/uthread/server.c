#include "ut_socket.h"
#include "uthread/uthread.h"
#include <stdio.h>
#include <stdlib.h>
#include "kn_time.h"

kn_sockaddr local;

int client_count = 0;
int packet_count = 0;

void uthread_worker(void *arg){
	++client_count;
	ut_socket_t client = *(ut_socket_t*)arg;
	free(arg);
	packet_t packet = NULL;
	int err = 0;
	while(ut_recv(client,&packet,&err) == 0){
		packet_t wpk = make_writepacket(packet);
		++packet_count;
		destroy_packet(packet);
		ut_send(client,wpk);
	}
	--client_count;
}


void uthread_accept(void *arg){
	printf("uthread_accept\n");
	ut_socket_t server = *(ut_socket_t*)arg;
	ut_socket_t *client = calloc(1,sizeof(*client));
	while(1){
		*client = ut_accept(server,4096,new_rpk_decoder(4096));
		if(!is_empty_ident(*client)){
			printf("new client\n");
			ut_spawn(uthread_worker,(void*)client);
			client = calloc(1,sizeof(*client));
		}
	}
}

void uthread_timer(void *_){
	((void)_);
	uint32_t tick = kn_systemms();
	while(1){
		ut_sleep(1000);
		uint32_t now = kn_systemms();
		uint32_t elapse = (uint32_t)(now-tick);
		printf("client_count:%d,packet_count:%d\n",client_count,packet_count/elapse*1000);
		packet_count = 0;
		tick = now;
	}
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	engine_t p = kn_new_engine();
	ut_socket_init(p);
	uscheduler_init(8192);
	ut_socket_t server = ut_socket_listen(&local);
	ut_spawn(uthread_accept,(void*)&server);
	ut_spawn(uthread_timer,NULL);
	ut_socket_run();
	return 0;
}
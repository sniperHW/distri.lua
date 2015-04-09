#include "kendynet.h"
#include "connection.h"
#include "kn_timer.h"

int  client_count = 0; 
int  packet_count = 0;

#define MAX_CLIENTS 500

connection_t clients[MAX_CLIENTS];


void  on_packet(connection_t c,packet_t p){
	int i = 0;
	for(; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			connection_send(clients[i],(packet_t)make_writepacket(p));
			++packet_count;
		}
	}
}

void on_disconnected(connection_t c,int err){
	printf("on_disconnectd\n");	
	int i = 0;
	for(; i < MAX_CLIENTS; ++i){
		if(clients[i] == c){
			clients[i] = NULL;
			break;
		}
	}
	--client_count;
}

void on_accept(handle_t s,void *ud){
	printf("on_accept\n");
	engine_t p = (engine_t)ud;
	connection_t conn = new_connection(s,4096,new_rpk_decoder(4096));
	connection_associate(p,conn,on_packet,on_disconnected);
	int i = 0;
	for(; i < MAX_CLIENTS; ++i){
		if(clients[i] == NULL){
			clients[i] = conn;
			break;
		}
	}	
	++client_count;
	printf("%d\n",client_count);   
}


int timer_callback(kn_timer_t timer){
	printf("client_count:%d,packet_count:%d\n",client_count,packet_count);
	packet_count = 0;
	return 1;
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sock_listen(p,l,&local,on_accept,p);
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}

#include "ioworker.h"
#include "logicprocessor.h"
#include "kendynet.h"
#include "connection.h"

static void Send(kn_thread_mailbox_t io,ident conn,packet_t p){
	msg_t _msg = new_msg(MSG_PACKET,conn,p,NULL);
	if(0 != kn_send_mail(io,_msg,destroy_msg)){
		destroy_msg(_msg);
	}
}


static void on_new_connection(kn_thread_mailbox_t ioworker,ident conn){
	printf("new connection\n");
}

static void on_packet(kn_thread_mailbox_t ioworker,ident conn,packet_t packet){
	packet_t wpk = make_writepacket(packet);
	//just echo the packet
	Send(ioworker,conn,wpk);
}

static void on_disconnected(kn_thread_mailbox_t ioworker,ident conn,int err){
	printf("disconnected\n");
}

worker_t _worker;

void on_accept(handle_t s,void *listener,int _2,int _3){

	connection_t conn = new_connection(s,4096,NULL);
	msg_t _msg = new_msg(MSG_CONNECTION,make_ident((refobj*)conn),NULL,NULL);
	ioworker_sendmsg(_worker,_msg);	
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);	
	//create ioworker and logicprocessor
	logicprocessor_t logic = create_logic(on_new_connection,on_packet,on_disconnected);
	_worker = ioworker_new(logic);
	//run net worker thread
	ioworker_startrun(_worker);
	//run logic process thread
	logic_startrun(logic);

	//start listen
	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(0 == kn_sock_listen(l,&local)){
		kn_engine_associate(p,l,on_accept);
		kn_engine_run(p);
	}
	return 0;
}
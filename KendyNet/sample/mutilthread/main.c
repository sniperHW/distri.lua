#include "ioworker.h"
#include "logicprocessor.h"
#include "kendynet.h"
#include "stream_conn.h"

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
	packet_t wpk = rpk_copy_create(packet);
	Send(ioworker,conn,wpk);
}

static void on_disconnected(kn_thread_mailbox_t ioworker,ident conn,int err){
	printf("disconnected\n");
}

void on_accept(handle_t s,void *ud){
	worker_t _worker = (worker_t)ud;
	stream_conn_t conn = new_stream_conn(s,4096,NULL);
	msg_t _msg = new_msg(MSG_CONNECTION,make_ident((refobj*)conn),NULL,NULL);
	ioworker_sendmsg(_worker,_msg);	
}

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);	
	//create ioworker and logicprocessor
	logicprocessor_t logic = create_logic(on_new_connection,on_packet,on_disconnected);
	worker_t _worker = ioworker_new(logic);

	engine_t p = kn_new_engine();
	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sock_listen(p,l,&local,on_accept,_worker);

	ioworker_startrun(_worker);
	logic_startrun(logic);
	kn_engine_run(p);

	return 0;
}
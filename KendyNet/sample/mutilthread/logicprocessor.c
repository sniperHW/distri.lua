#include "logicprocessor.h"
#include "ioworker.h"

static __thread logicprocessor_t t_logic = NULL;

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	msg_t _msg = (msg_t)mail;
	do{
		switch(_msg->_msg_type){
			case MSG_CONNECTION:{
				if(t_logic->on_new_connection)
					t_logic->on_new_connection(*from,_msg->_stream_conn);
				break;
			}
			case MSG_PACKET:{
				t_logic->on_packet(*from,_msg->_stream_conn,_msg->_packet);
				break;
			}
			case MSG_CLOSED:{
				if(t_logic->on_disconnected)
					t_logic->on_disconnected(*from,_msg->_stream_conn,(int)((uint64_t)_msg->data));
				break;
			}
			default:
				break;
		}
	}while(0);
}

static void* logic_routine(void *_){
	printf("logic_routine\n");	
	logicprocessor_t logic = (logicprocessor_t)_;
	t_logic = logic;
	engine_t _engine = (engine_t)logic->_engine;
	kn_setup_mailbox(_engine,on_mail);
	logic->_mailbox = kn_self_mailbox();
	kn_engine_run(_engine);
	return NULL;
}

void send2logic(logicprocessor_t logic,msg_t msg){
	while(is_empty_ident(logic->_mailbox))
		FENCE();
	if(0 != kn_send_mail(logic->_mailbox,msg,destroy_msg)){
		destroy_msg(msg);
	}
}

logicprocessor_t create_logic(void (*on_new_connection)(kn_thread_mailbox_t,ident),
			          void (*on_packet)(kn_thread_mailbox_t,ident,packet_t),
			          void (*on_disconnected)(kn_thread_mailbox_t,ident,int)){
	if(!on_packet){
		return NULL;
	}
	logicprocessor_t logic = calloc(1,sizeof(*logic));
	logic->_engine = kn_new_engine();
	logic->on_new_connection = on_new_connection;
	logic->on_packet = on_packet;
	logic->on_disconnected = on_disconnected;
	return logic;
}

void logic_startrun(logicprocessor_t logic){
	if(!logic->_thread){
		logic->_thread = kn_create_thread(THREAD_JOINABLE);
		kn_thread_start_run(logic->_thread,logic_routine,logic);
	}
}
void stop_logic(logicprocessor_t logic){
	if(logic->_thread){
		kn_stop_engine(logic->_engine);
		kn_thread_join(logic->_thread);
		kn_thread_destroy(logic->_thread);
		logic->_thread = NULL;
	}	
}

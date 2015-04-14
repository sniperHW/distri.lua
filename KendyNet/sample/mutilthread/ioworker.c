#include "ioworker.h"
#include "kn_thread.h"
#include "logicprocessor.h"
#include "connection.h"

static __thread worker_t t_worker = NULL;


static void sendmsg2logic(msg_t msg){
	send2logic(t_worker->_logicprocessor,msg);
}

static void  on_packet(connection_t c,packet_t _p){	
	packet_t p = clone_packet(_p);
	msg_t _msg = new_msg(MSG_PACKET,make_ident((refobj*)c),p,NULL);
	sendmsg2logic(_msg);	
}

static void on_disconnected(connection_t c,int err){
	//notify logic disconnected
	msg_t _msg = new_msg(MSG_CLOSED,make_ident((refobj*)c),NULL,(void*)((uint64_t)err));
	sendmsg2logic(_msg);
}

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	msg_t _msg = (msg_t)mail;
	do{
		connection_t conn = (connection_t)cast2refobj(_msg->_stream_conn);
		if(!conn){ 
			break;
		}
		switch(_msg->_msg_type){
			case MSG_CONNECTION:{
				connection_associate(t_worker->_engine,conn,on_packet,on_disconnected);
				msg_t _msg1 = new_msg(MSG_CONNECTION,_msg->_stream_conn,NULL,NULL);
				sendmsg2logic(_msg1);				
				break;
			}
			case MSG_PACKET:{
				connection_send(conn,_msg->_packet,NULL);
				_msg->_packet = NULL;
				break;
			}
			case MSG_ACTIVECLOSE:{
				connection_close(conn);
				break;
			}
			default:
				break;
		}
		refobj_dec((refobj*)conn);
	}while(0);
}

static void* worker_routine(void *_){
	printf("worker_routine\n");
	worker_t _worker  = (worker_t)_;
	t_worker = _worker;
	engine_t _engine = (engine_t)_worker->_engine;
	kn_setup_mailbox(_engine,on_mail);
	_worker->_mailbox = kn_self_mailbox();
	kn_engine_run(_engine);
	return NULL;
}

worker_t ioworker_new(struct logicprocessor *_logicprocessor){
	worker_t _worker = calloc(1,sizeof(*_worker));
	_worker->_engine = kn_new_engine();
	_worker->_logicprocessor = _logicprocessor;
	return _worker;
}

void ioworker_sendmsg(worker_t _worker,msg_t msg){
	while(is_empty_ident(_worker->_mailbox))
		FENCE();
	if(0 != kn_send_mail(_worker->_mailbox,msg,destroy_msg)){
		destroy_msg(msg);
	}
}

void ioworker_startrun(worker_t _worker){
	if(!_worker->_thread){
		_worker->_thread = kn_create_thread(THREAD_JOINABLE);
		kn_thread_start_run(_worker->_thread,worker_routine,_worker);
	}
}

void ioworker_stop(worker_t _worker){
	if(_worker->_thread){
		kn_stop_engine(_worker->_engine);
		kn_thread_join(_worker->_thread);
		kn_thread_destroy(_worker->_thread);
		_worker->_thread = NULL;
	}
}
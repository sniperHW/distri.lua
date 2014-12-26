#include "ioworker.h"
#include "kn_thread.h"

static __thread worker_t t_worker = NUL;

static int  on_packet(stream_conn_t c,packet_t p){	
	return 1;
}

static void on_disconnected(stream_conn_t c,int err){

}

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	msg_t _msg = (msg_t)mail;
	do{
		stream_conn_t conn = (stream_conn_t)cast2refobj(_msg->_stream_conn);
		if(!conn){ 
			break;
		}
		switch(_msg->_msg_type){
			case MSG_CONNECTION:{
				stream_conn_associate(t_worker->_engine,conn,on_packet,on_disconnected);
				break;
			}
			case MSG_PACKET:{
				stream_conn_send(conn,_msg->_packet);
				_msg->_packet = NULL;
				break;
			}
			case MSG_ACTIVECLOSE:{

				break;
			}
			default:
				break;
		}
		refobj_dec((ref_obj*)conn);
	}while(0);	
	destroy_msg(_msg);
}

static void* worker_routine(void *_){
	worker_t _worker  = (worker_t)_;
	t_worker = _worker;
	engine_t _engine = (engine_t)_worker->_engine;
	kn_setup_mailbox(_engine,on_mail);
	_worker->_mailbox = kn_self_mailbox();
	kn_engine_run(_engine);
}

worker_t ioworker_new(kn_thread_mailbox_t _logicprocessor){
	worker_t _worker = calloc(1,sizeof(*_worker));
	_worker->_engine = kn_new_engine();
	return _worker;
}

void ioworker_sendmsg(worker_t _worker,msg_t msg){
	while(is_empty_ident(_worker->_mailbox))
		FENCE();
	if(0 != kn_send_mail(_worker->_mailbox,msg,destroy_msg)){
		destroy_msg(msg);
	}
}

void ioworker_run(worker_t _worker){
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
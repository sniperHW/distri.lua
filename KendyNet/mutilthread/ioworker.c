#include "ioworker.h"
#include "kn_thread.h"

static int  on_packet(stream_conn_t c,packet_t p){	
	return 1;
}

static void on_disconnected(stream_conn_t c,int err){

}

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	msg_t _msg = (msg_t)mail;
	switch(_msg->_msg_type){
		case MSG_CONNECTION:{

			break;
		}
		case MSG_PACKET:{

			break;
		}
		case MSG_ACTIVECLOSE:{

			break;
		}
		default:
			break;
	}	
	destroy_msg(_msg);
}

static void* worker_routine)(void *_){
	worker_t _worker  = (worker_t)_;
	engine_t _engine = (engine_t)_worker->_engine;

	


	kn_engine_run(_engine);
}

worker_t ioworker_new(kn_thread_mailbox_t _logicprocessor){
	worker_t _worker = calloc(1,sizeof(*_worker));
	_worker->_engine = kn_new_engine();

}

void ioworker_run(worker_t);
void ioworker_stop(worker_t);
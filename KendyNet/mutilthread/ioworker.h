#ifndef  _IOWORKER_H
#define _IOWORKER_H

#include "kendynet.h"
#include "kn_thread_mailbox.h"
#include "msg.h"

typedef struct{
	engine_t _engine;
	kn_thread_mailbox_t _logicprocessor;
	kn_thread_mailbox_t _mailbox;
}ioworker,*worker_t;

worker_t ioworker_new(kn_thread_mailbox_t _logicprocessor);
void ioworker_run(worker_t);
void ioworker_stop(worker_t);
void ioworker_sendmsg(worker_t,msg_t);

#endif
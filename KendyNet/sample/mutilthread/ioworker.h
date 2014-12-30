#ifndef  _IOWORKER_H
#define _IOWORKER_H

#include "kendynet.h"
#include "kn_thread.h"
#include "kn_thread_mailbox.h"
#include "msg.h"

struct logicprocessor;

typedef struct{
	engine_t _engine;
	struct logicprocessor* _logicprocessor;
	kn_thread_mailbox_t  _mailbox;
	kn_thread_t _thread;
}*worker_t;

worker_t ioworker_new(struct logicprocessor *_logicprocessor);
void ioworker_startrun(worker_t);
void ioworker_stop(worker_t);
void ioworker_sendmsg(worker_t,msg_t);

#endif
#ifndef  _LOGICPROCESSOR_H
#define _LOGICPROCESSOR_H
#include "kendynet.h"
#include "kn_thread.h"
#include "kn_thread_mailbox.h"
#include "msg.h"
#include "ioworker.h"

typedef struct logicprocessor{
	engine_t _engine;
	kn_thread_mailbox_t  _mailbox;
	kn_thread_t _thread;
	void (*on_new_connection)(kn_thread_mailbox_t,ident);
	void (*on_packet)(kn_thread_mailbox_t,ident,packet_t);
	void (*on_disconnected)(kn_thread_mailbox_t,ident,int);	
}*logicprocessor_t;

void send2logic(logicprocessor_t,msg_t);
logicprocessor_t create_logic(void (*on_new_connection)(kn_thread_mailbox_t,ident),
			         void (*on_packet)(kn_thread_mailbox_t,ident,packet_t),
			         void (*on_disconnected)(kn_thread_mailbox_t,ident,int));
void logic_startrun(logicprocessor_t);
void stop_logic(logicprocessor_t);

#endif
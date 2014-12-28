#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "stream_conn.h"
#include "kn_thread_mailbox.h"

typedef struct {
	ident _stream_conn;
	kn_thread_mailbox_t _mailbox_worker;
}connection,*connection_t;

void Send(connection_t c,packet_t p);

void ActiveClose(connection_t c);

#endif


#ifndef _MSG_H
#define _MSG_H
#include "kn_refobj.h"
#include "packet.h"

enum{
	MSG_PACKET,
	MSG_ACTIVECLOSE,
	MSG_CLOSED,
	MSG_CONNECTION,
}

typedef struct{
	uint16_t _msg_type;
	ident _stream_conn;
	packet_t _packet;	
}msg,*msg_t;

static inline void destroy_msg(void *_msg){
	free(_msg);
} 

#endif
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

static inline msg_t new_msg(uint16_t _type,ident *_conn,packet_t _packet){
	_msg_t _msg = calloc(1,sizeof(*_msg));
	_msg->_msg_type = _type;
	if(_conn) _msg->_stream_conn = *_conn;
	if(_packet) _msg->_packet = _packet;
}

static inline void destroy_msg(void *_msg){
	if(_msg->_packet){
		destroy_packet(_msg->_packet);
	}
	free(_msg);
} 

#endif
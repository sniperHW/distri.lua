#include "connection.h"
#include "msg.h"

void Send(connection_t c,packet_t p){
	msg_t _msg = calloc(1,sizeof(*_msg));
	_msg->_msg_type = MSG_PACKET;
	_msg->ident = c->_stream_conn;
	int ret = kn_send_mail(_msg->_mailbox_worker,_msg,destroy_msg);
	if(ret != 0){
		destroy_packet(p);
		destroy_msg((void*)_msg);
	}
	return ret;
}

void ActiveClose(connection_t c){
	msg_t _msg = calloc(1,sizeof(*_msg));
	_msg->_msg_type = MSG_ACTIVECLOSE;
	_msg->ident = c->_stream_conn;	
	int ret = kn_send_mail(_msg->_mailbox_worker,_msg,destroy_msg);
	if(ret != 0){
		destroy_msg((void*)_msg);
	}
	return ret;
}
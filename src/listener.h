#ifndef _LISTENER_H
#define _LISTENER_H

#include "kendynet.h"
#include "luasocket.h"

enum{
	MSG_CONNECTION,       //listener -> main, notify a new connection
	MSG_CLOSE,                    //main -> listener,  notify to close a listen socket
	MSG_CLOSED,                 //listener -> main,  notify close listen socket success
};

typedef struct li_msg{
	uint16_t _msg_type;
	luasocket_t luasock;
	void *ud;
}*li_msg_t;

int    listener_listen(luasocket_t,kn_sockaddr *addr);
void listener_close_listen(luasocket_t);
void listener_stop(); 

#endif
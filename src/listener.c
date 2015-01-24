#include "kn_thread_mailbox.h"
#include "kn_thread.h"
#include "listener.h"
#include <assert.h>

typedef struct{
	engine_t engine;
	kn_thread_mailbox_t   mailbox;
	kn_thread_t thread;
	kn_thread_mailbox_t   mainmailbox;
}*listener_t;

static inline li_msg_t new_msg(uint16_t _type,luasocket_t luasock,void *ud)
{
	li_msg_t _msg = calloc(1,sizeof(*_msg));
	_msg->luasock = luasock;
	_msg->ud = ud;
	return _msg;
}

static inline void destroy_msg(void *_){
	free(_);
}

static listener_t g_listener = NULL;

static void send2listener(li_msg_t msg){
	while(is_empty_ident(g_listener->mailbox))
		FENCE();
	if(0 != kn_send_mail(g_listener->mailbox,msg,destroy_msg)){
		destroy_msg(msg);
	}
}

static void send2main(li_msg_t msg){
	if(0 != kn_send_mail(g_listener->mainmailbox,msg,destroy_msg)){
		destroy_msg(msg);
	}
}

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	li_msg_t _msg = (li_msg_t)mail;
	if(_msg->_msg_type == MSG_CLOSE){
		luasocket_t luasock = (luasocket_t)_msg->luasock;
		kn_close_sock(luasock->sock);
		send2main(new_msg(MSG_CLOSED,_msg->luasock,NULL));
	}
}

static void* routine(void *_){
	printf("listener running\n");
	kn_setup_mailbox(g_listener->engine,on_mail);
	g_listener->mailbox = kn_self_mailbox();
	kn_engine_run(g_listener->engine);
	return NULL;
}

static void on_new_conn(handle_t s,void* ud){
	luasocket_t luasock = (luasocket_t)ud;
	send2main(new_msg(MSG_CLOSED,luasock,s));	
}

int    listener_listen(luasocket_t lsock,kn_sockaddr *addr){
	//must be call in main thread
	if(!g_listener){
		g_listener = calloc(1,sizeof(*g_listener));
		g_listener->engine = kn_new_engine();
		g_listener->thread = kn_create_thread(THREAD_JOINABLE);
		g_listener->mainmailbox = kn_self_mailbox();//get mailbox of main thread
		kn_thread_start_run(g_listener->thread,routine,NULL);
	}
	return kn_sock_listen(g_listener->engine,lsock->sock,addr,on_new_conn,lsock);
}

void listener_close_listen(luasocket_t lsock){
	assert(g_listener);
	send2listener(new_msg(MSG_CLOSE,lsock,NULL));
}

void listener_stop(){
	if(g_listener){
		kn_stop_engine(g_listener->engine);
		kn_thread_join(g_listener->thread);
	}
}


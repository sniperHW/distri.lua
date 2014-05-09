#ifndef _KN_CHANNEL_H
#define _KN_CHANNEL_H

/*线程间通信的单向信道,可多个线程同时从中读取消息*/

#include "kn_fd.h"
#include "kn_thread_sync.h"
#include "spinlock.h"
#include "kn_list.h"
#include "kn_dlist.h"
#include "kn_common_define.h"

struct kn_proactor;
typedef ident kn_channel_t;

typedef struct kn_channel{
	kn_ref        ref;
	pthread_key_t t_key;
	LOCK_TYPE     lock;
	kn_list       queue;
	kn_dlist      waits;
	pthread_t     owner;
	ident         ident;	
}kn_channel;

struct channel_pth{
	kn_fd         base;
	kn_dlist_node node;
	int           notifyfd;	
	kn_list       local_que;
	kn_channel*   channel;
	void*         ud;
	//接收到消息后回调用
	void  (*cb_msg)(kn_channel_t, kn_channel_t sender,void*,void*);		 
};

#endif

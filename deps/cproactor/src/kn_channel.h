#ifndef _KN_CHANNEL_H
#define _KN_CHANNEL_H

/*线程间通信的单向信道,可多个线程同时从中读取消息*/

#include "kn_fd.h"
#include "kn_thread_sync.h"
#include "kn_list.h"
#include "kn_dlist.h"
#include "kn_notifyer.h"

struct kn_proactor;

typedef struct kn_channel{
	pthread_key_t t_key;
	kn_mutex      mtx;
	kn_list       queue;
	kn_dlist      waits;
	//接收到消息后回调用
	void  (*cb_msg)(struct kn_channel *, struct kn_channel *sender,void*);	
}kn_channel,*kn_channel_t;

struct channel_pth{
	kn_fd         base;
	kn_dlist_node node;
	int           notifyfd;	
	kn_list       local_que;
	kn_channel_t  channel;	 
};

kn_channel_t kn_new_channel(void(*)(struct kn_channel*, struct kn_channel*,void*));

int kn_channel_bind(struct kn_proactor*,kn_channel_t);

/*
*  from:如果不为空,表示如果要对这条消息作响应响应消息发往from. 
*/ 
void kn_channel_putmsg(kn_channel_t to,kn_channel_t from,void*);

#endif

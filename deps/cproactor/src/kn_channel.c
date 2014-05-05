#include "kn_channel.h"
#include <assert.h>

kn_channel_t kn_new_channel(void(*cb_msg)(struct kn_channel*, struct kn_channel*,void*)){
	assert(cb_msg);
	kn_channel_t c = calloc(1,sizeof(*c));
	c->mtx = kn_mutex_create();
	pthread_key_create(&c->t_key,NULL);
	kn_dlist_init(&c->waits);
	c->cb_msg = cb_msg;
	return c;
}

static void kn_channel_on_active(kn_fd_t s,int event){
	channel_pth* c = (channel_pth*)s;
	char buf[4096];
	int ret;
	if(event & EPOLLIN){
		while((ret = TEMP_FAILURE_RETRY(read(s->fd,buf,4096))) > 0);
		kn_procator_putin_active(s->proactor,s);
	}
}

struct msg{
	kn_list_node node;
	kn_channel_t sender;
	void *data;	
};

void kn_channel_putmsg(kn_channel_t to,kn_channel_t from,void *data)
{
	kn_dlist_node *tmp = NULL;
	struct msg *msg = calloc(1,sizeof(*msg));
	msg->sender = from;
	msg->data = data;
	kn_mutex_lock(to->mtx);
	kn_list_pushback(to->queue,&msg->node);
	tmp = kn_dlist_first(to->waits);
	if(tmp){
		//有线程在等待消息，通知它有消息到了
		struct channel_pth *pth = (struct channel_pth*)(tmp-sizeof(kn_fd));
		write(pth->notifyfd,"",1);
	}
	kn_mutex_unlock(to->mtx);		
}

static inline struct msg* kn_channel_getmsg(channel_pth *c){
	struct msg *msg = (struct msg*)kn_list_pop(&c->local_que);
	if(msg) return msg;
	kn_mutex_lock(to->mtx);
	if(!kn_list_size(&c->channel->queue)){
		kn_dlist_push(&c->channel->waits,&c->node);
		kn_mutex_unlock(to->mtx);
		return NULL;
	}else{
		kn_list_swap(&c->local_que,&c->channel->queue);
	}
	kn_mutex_unlock(to->mtx);
	return kn_list_pop(&c->local_que);
}

static int8_t kn_channel_process(kn_fd_t s){
	channel_pth* c = (channel_pth*)s;
	struct msg *msg;
	int c = 1024;
	while((msg = kn_channel_getmsg(c)) != NULL && c > 0){
		c->channel->cb_msg(msg->sender,msg->data);
		free(msg->data);
		free(msg);
	}
	if(c == 0) 
		return 1;
	else 
		return 0;	
}

static void channel_pth_destroy(void *ptr);{
	//kn_notifyer_t *n = (kn_notifyer_t *)((char*)ptr - sizeof(kn_dlist_node));
	//close(t->base.fd);
	//close(n->notifyfd);	
	//free(t);
}


int kn_channel_bind(struct kn_proactor *p,kn_channel_t c){
	struct channel_pth_st *pth = (struct channel_pth_st*)pthread_getspecific(c->t_key);
	if(pth) return -1;
	pth = calloc(1,sizeof(*pth));
		
	pth->base.type = CHANNEL;
	int p[2];
	if(pipe(p) != 0){ 
		free(pth);
		return -1;
	}
	pth->base.fd = p[0];
	pth->notifyfd = p[1];
	pth->base.on_active = kn_channel_on_active;		
	pth->base.process = kn_channel_process;
	kn_ref_init(&pth->base.ref,channel_pth_destroy);		
	if(0!= p->Register(p,(kn_fd_t)pth)){
		kn_ref_release((kn_ref*)pth);
		return -1;
	}
	pthread_setspecific(c->t_key,(void*)pth);
	return 0;
}

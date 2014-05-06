#include "kn_channel.h"
#include "kn_proactor.h"
#include <assert.h>

struct msg{
	kn_list_node node;
	kn_channel_t sender;
	void *data;	
};

static void channel_destroy(void *ptr){
	kn_channel_t c = (kn_channel_t)ptr;
	struct msg* msg;
	while((msg = (struct msg*)kn_list_pop(&c->queue)) != NULL){
		if(msg->sender) kn_ref_release(&msg->sender->ref);
		free(msg->data);
		free(msg); 
	}	
	kn_mutex_destroy(c->mtx);
	pthread_key_delete(c->t_key);
	free(c);
}

void kn_channel_close(kn_channel_t c){
	struct channel_pth_st *pth = (struct channel_pth_st*)pthread_getspecific(c->t_key);
	if(pth){
		pthread_setspecific(c->t_key,NULL);
		kn_ref_release((kn_ref*)pth);
	}
	if(pthread_self() == c->owner)
		kn_ref_release((kn_ref*)c);
}

kn_channel_t kn_new_channel(pthread_t owner){
	kn_channel_t c = calloc(1,sizeof(*c));
	c->mtx = kn_mutex_create();
	pthread_key_create(&c->t_key,NULL);
	kn_dlist_init(&c->waits);
	kn_ref_init(&c->ref,channel_destroy);
	c->owner = owner;
	return c;
}

static void kn_channel_on_active(kn_fd_t s,int event){
	//struct channel_pth* c = (struct channel_pth*)s;
	char buf[4096];
	int ret;
	if(event & EPOLLIN){
		/*kn_mutex_lock(c->channel->mtx);
		kn_dlist_remove(&c->node);
		kn_mutex_unlock(c->channel->mtx);
		*/
		while((ret = TEMP_FAILURE_RETRY(read(s->fd,buf,4096))) > 0);
		if(ret == 0 || (ret < 0 && errno != EAGAIN)){
			//channel被关闭
			return;
		}
		kn_procator_putin_active(s->proactor,s);
	}
}

void kn_channel_putmsg(kn_channel_t to,kn_channel_t from,void *data)
{
	kn_dlist_node *tmp = NULL;
	int ret = 0;
	struct msg *msg = calloc(1,sizeof(*msg));
	msg->sender = from;
	msg->data = data;
	if(from){
		//防止消息发送后from被释放掉
		kn_ref_acquire(&from->ref);
	}
	kn_mutex_lock(to->mtx);
	kn_list_pushback(&to->queue,&msg->node);
	while(1){
		tmp = kn_dlist_first(&to->waits);
		if(tmp){
			//有线程在等待消息，通知它有消息到了
			struct channel_pth *pth = (struct channel_pth*)(((char*)tmp)-sizeof(kn_fd));
			ret = write(pth->notifyfd,"",1);
			kn_dlist_pop(&to->waits);
			if(!(ret == 0 || (ret < 0 && errno != EAGAIN)))
				break;
			/*if(ret == 0 || (ret < 0 && errno != EAGAIN)){
				//对端关闭
				kn_dlist_pop(&to->waits);
			}else
				break;*/
		}else
			break;
	};
	kn_mutex_unlock(to->mtx);		
}

static inline struct msg* kn_channel_getmsg(struct channel_pth *c){
	struct msg *msg = (struct msg*)kn_list_pop(&c->local_que);
	if(msg) return msg;
	kn_mutex_lock(c->channel->mtx);
	if(!kn_list_size(&c->channel->queue)){
		kn_dlist_push(&c->channel->waits,&c->node);
		kn_mutex_unlock(c->channel->mtx);
		return NULL;
	}else{
		kn_list_swap(&c->local_que,&c->channel->queue);
	}
	kn_mutex_unlock(c->channel->mtx);
	return (struct msg*)kn_list_pop(&c->local_que);
}

static int8_t kn_channel_process(kn_fd_t s){
	struct channel_pth* c = (struct channel_pth*)s;
	struct msg *msg;
	int n = 1024;
	while((msg = kn_channel_getmsg(c)) != NULL && n > 0){
		c->cb_msg(c->channel,msg->sender,msg->data,c->ud);
		if(msg->sender) kn_ref_release(&msg->sender->ref);
		free(msg->data);
		free(msg);
		--n;
	}
	if(n <= 0) 
		return 1;
	else 
		return 0;	
}

static void channel_pth_destroy(void *ptr){
	struct msg *msg;
	struct channel_pth *pth = (struct channel_pth*)ptr;
	close(pth->base.fd);
	close(pth->notifyfd);	
	while((msg = (struct msg*)kn_list_pop(&pth->local_que)) != NULL){
		if(msg->sender) kn_ref_release(&msg->sender->ref);
		free(msg->data);
		free(msg); 
	}
	if(pth->base.proactor)
		pth->base.proactor->UnRegister(pth->base.proactor,&pth->base);	
	kn_ref_release((kn_ref*)pth->channel);
	free(ptr);
}

int kn_channel_bind(struct kn_proactor *p,kn_channel_t c,
					void(*cb_msg)(struct kn_channel*, struct kn_channel*,void*,void*),
					void *ud)
{
	assert(cb_msg);
	struct channel_pth *pth = (struct channel_pth*)pthread_getspecific(c->t_key);
	if(pth) return -1;
	pth = calloc(1,sizeof(*pth));
		
	pth->base.type = CHANNEL;
	int tmp[2];
	if(pipe(tmp) != 0){ 
		free(pth);
		return -1;
	}
	pth->base.fd = tmp[0];
	pth->notifyfd = tmp[1];
	pth->base.on_active = kn_channel_on_active;		
	pth->base.process = kn_channel_process;
	pth->channel = c;
	pth->ud = ud;
	fcntl(tmp[0], F_SETFL, O_NONBLOCK | O_RDWR);
	fcntl(tmp[1], F_SETFL, O_NONBLOCK | O_RDWR);
	kn_ref_init(&pth->base.ref,channel_pth_destroy);		
	if(0!= p->Register(p,(kn_fd_t)pth)){
		kn_ref_release((kn_ref*)pth);
		return -1;
	}
	pth->cb_msg = cb_msg;	
	kn_ref_acquire(&c->ref);
	pthread_setspecific(c->t_key,(void*)pth);
	kn_procator_putin_active(p,(kn_fd_t)pth);
	return 0;
}



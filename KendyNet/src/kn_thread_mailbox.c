#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include "kn_thread_mailbox.h"
#include "kn_thread_sync.h"
#include "spinlock.h"
#include "kn_list.h"
#include "kn_type.h"
#include "kendynet_private.h"
#include "kn_event.h"

#ifdef USE_MUTEX
#define LOCK_TYPE kn_mutex_t
#define LOCK(L) kn_mutex_lock(L)
#define UNLOCK(L) kn_mutex_unlock(L)
#define LOCK_CREATE kn_mutex_create
#define LOCK_DESTROY(L) kn_mutex_destroy(L) 
#else
#define LOCK_TYPE spinlock_t
#define LOCK(L) spin_lock(L)
#define UNLOCK(L) spin_unlock(L)
#define LOCK_CREATE spin_create 
#define LOCK_DESTROY(L) spin_destroy(L) 
#endif	

typedef struct kn_thread_mailbox{
	handle         comm_head;
	refobj         refobj;
	LOCK_TYPE      mtx;
	kn_list        global_queue;
	kn_list        private_queue;
	int            wait;
	int            emptytry;
	int            notifyfd;
	engine_t       e;
	void (*cb_on_mail)(kn_thread_mailbox_t *from,void*);          
}kn_thread_mailbox;

struct mail{
	kn_list_node        node;
	kn_thread_mailbox_t sender;
	void (*fn_destroy)(void*);
	void *data;	
};

static pthread_key_t  g_mailbox_key;	

#ifndef __GNUC__
static pthread_once_t g_mailbox_key_once = PTHREAD_ONCE_INIT;
#endif

static void key_destructor(void *ptr){
	kn_thread_mailbox* mailbox  = (kn_thread_mailbox*)ptr;
	refobj_dec(&mailbox->refobj); 
}

#ifndef __GNUC__
static void mailbox_once_routine(){
#else
__attribute__((constructor(102))) static void mailbox_init(){
#endif
	pthread_key_create(&g_mailbox_key,key_destructor);
}

static void mailbox_destroctor(void *ptr){
	kn_thread_mailbox* mailbox = (kn_thread_mailbox*)((char*)ptr - sizeof(handle));
	struct mail* mail;
	while((mail = (struct mail*)kn_list_pop(&mailbox->private_queue))){
		if(mail->fn_destroy) mail->fn_destroy(mail->data);
		free(mail);		
	}
	while((mail = (struct mail*)kn_list_pop(&mailbox->global_queue))){
		if(mail->fn_destroy) mail->fn_destroy(mail->data);
		free(mail);		
	}	
	kn_event_del(mailbox->e,(handle_t)mailbox);
	LOCK_DESTROY(mailbox->mtx);	
	close(mailbox->comm_head.fd);
	close(mailbox->notifyfd);	
	free(mailbox);			
}

#define MAX_EMPTY_TRY 16
#define MAX_EVENTS 512
static inline  struct mail* kn_getmail(kn_thread_mailbox *mailbox){
	if(!kn_list_size(&mailbox->private_queue)){
		LOCK(mailbox->mtx);
		if(!kn_list_size(&mailbox->global_queue)){
			++mailbox->emptytry;
			if(mailbox->emptytry > MAX_EMPTY_TRY){
				uint64_t _;
				TEMP_FAILURE_RETRY(read(mailbox->comm_head.fd,&_,sizeof(_)));
				mailbox->emptytry = 0;
				mailbox->wait = 1;
			}
			UNLOCK(mailbox->mtx);
			return NULL;
		}else{
			mailbox->emptytry = 0;
			kn_list_swap(&mailbox->private_queue,&mailbox->global_queue);
		}
		UNLOCK(mailbox->mtx);
	}
	return (struct mail*)kn_list_pop(&mailbox->private_queue);
}

static void on_events(handle_t h,int events){
	kn_thread_mailbox *mailbox = (kn_thread_mailbox*)h;
	struct mail *mail;
	int n = MAX_EVENTS;
	while((mail = kn_getmail(mailbox)) != NULL){
		kn_thread_mailbox_t *sender = NULL;
		if(mail->sender.ptr) sender = &mail->sender;
		mailbox->cb_on_mail(sender,mail->data);
		if(mail->fn_destroy) mail->fn_destroy(mail->data);
		free(mail);	
		if(--n == 0) break;
	}
}

static kn_thread_mailbox* create_mailbox(engine_t e,cb_on_mail cb){	
	int tmp[2];
	if(pipe2(tmp,O_NONBLOCK|O_CLOEXEC) != 0){
		NULL;
	}		
	kn_thread_mailbox *mailbox = calloc(1,sizeof(*mailbox));
	mailbox->comm_head.fd = tmp[0];
	mailbox->notifyfd = tmp[1];	
	refobj_init(&mailbox->refobj,mailbox_destroctor);
#ifdef _LINUX	
	if(kn_event_add(e,(handle_t)mailbox,EPOLLIN) != 0){
#elif _BSD
	if(kn_event_add(e,(handle_t)mailbox,EVFILT_READ) != 0){
#endif
		close(tmp[0]);
		close(tmp[1]);
		free(mailbox);
		return NULL;	
	}
	mailbox->comm_head.type = KN_MAILBOX;
	mailbox->comm_head.on_events = on_events;
	mailbox->mtx = LOCK_CREATE();
	mailbox->wait = 1;
	mailbox->cb_on_mail = cb;
	mailbox->e = e;

	kn_list_init(&mailbox->global_queue);
	kn_list_init(&mailbox->private_queue);

	return mailbox;	
}


kn_thread_mailbox_t kn_setup_mailbox(engine_t e,cb_on_mail cb){
	assert(e);
	assert(cb);
	kn_thread_mailbox_t mailbox = {.identity =0,.ptr=NULL};
	if(!e || !cb) return mailbox;
#ifndef __GNUC__
	pthread_once(&g_mailbox_key_once,mailbox_once_routine);
#endif	
	kn_thread_mailbox *m = (kn_thread_mailbox*)pthread_getspecific(g_mailbox_key);
	if(!m){
		m = create_mailbox(e,cb);
		if(!m){
			return mailbox;
		}
		pthread_setspecific(g_mailbox_key,m);
	}
	return make_ident(&m->refobj);
}

static inline kn_thread_mailbox* cast(kn_thread_mailbox_t t){
	refobj *tmp = cast2refobj(t);
	if(!tmp) return NULL;
	return (kn_thread_mailbox*)((char*)tmp - sizeof(handle));
}


static inline int notify(kn_thread_mailbox *mailbox){
	int ret;
	for(;;){
		errno = 0;
		uint64_t _ = 1;
		ret = TEMP_FAILURE_RETRY(write(mailbox->notifyfd,&_,sizeof(_)));
		if(!(ret < 0 && errno == EAGAIN))
			break;
	}
	return ret > 0 ? 0:-1;
}

int  kn_send_mail(kn_thread_mailbox_t mailbox,void *mail,void (*fn_destroy)(void*))
{
#ifndef __GNUC__
	pthread_once(&g_mailbox_key_once,mailbox_once_routine);
#endif	
	kn_thread_mailbox *targetbox = cast(mailbox);
	if(!targetbox) return -1;
	kn_thread_mailbox *selfbox = (kn_thread_mailbox*)pthread_getspecific(g_mailbox_key);	
	
	struct mail *msg  = calloc(1,sizeof(*msg));
	if(selfbox) msg->sender = make_ident(&selfbox->refobj);
	msg->data = mail;
	msg->fn_destroy = fn_destroy;
	int ret = 0;	
	if(selfbox == targetbox){
		do{
			if(targetbox->wait){
				LOCK(targetbox->mtx);
				if(targetbox->wait && (ret = notify(targetbox)) == 0)
					targetbox->wait = 0;
				UNLOCK(targetbox->mtx);
				if(ret != 0) break;
			}
			kn_list_pushback(&targetbox->private_queue,(kn_list_node*)msg);
		}while(0);	
	}else{		
		LOCK(targetbox->mtx);
		if(targetbox->wait && (ret = notify(targetbox)) == 0)
			targetbox->wait = 0;
		if(ret == 0)
			kn_list_pushback(&targetbox->global_queue,(kn_list_node*)msg);
		UNLOCK(targetbox->mtx);
	}
	refobj_dec(&targetbox->refobj);//cast会调用refobj_inc,所以这里要调用refobj_dec
	return ret;
}


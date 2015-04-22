#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>
#include <assert.h>
#include "kn_dlist.h"
#include "uthread/uthread.h"
#include "minheap.h"
#include "kn_common_define.h"
#include "kn_time.h"
#include "hash_map.h"
#include "common_hash_function.h"
//uthread status
enum{
	start,
	ready,
	running,
	blocking,
	sleeping,
	yield,
	dead,
};

struct uthread
{
   kn_dlist_node  node;
   refobj             refobj;	   
   struct heapele heapnode;	
   ucontext_t ucontext;
   uint32_t     timeout;	
   void *param;   		
   uint8_t       status;
   void   *stack;
   hash_map_t    dictionary;
   void (*main_fun)(void*);
};

#define MAX_KEY_SIZE 64

struct ut_dic_node{
	hash_node _hash_node;
	char            _key[MAX_KEY_SIZE];
	void          *value;
	void           (*value_destroyer)(void*);
};

typedef struct {
	kn_dlist ready_list;
	struct uthread *current;
	struct uthread *ut_scheduler;
	uint32_t      stacksize;
	minheap_t  timer;
	uint32_t       activecount;
}*utscheduler_t;

static __thread utscheduler_t t_scheduler = NULL;

static inline void uthread_switch(struct uthread* from,struct uthread* to)
{
    	swapcontext(&(from->ucontext),&(to->ucontext));
}

static void uthread_main_function(void *arg)
{
	struct uthread* u = (struct uthread*)arg;
	u->main_fun(u->param);
	u->status = dead;
	--t_scheduler->activecount;
	uthread_switch(u,t_scheduler->ut_scheduler);
}

static void hash_destroyer(hash_node *_node){
	struct ut_dic_node *node = (struct ut_dic_node*)_node;
	if(node->value_destroyer && node->value){
		node->value_destroyer(node->value);	
	}
	free(node);
}

void uthread_destroy(void *_u)
{
	struct uthread* u = (struct uthread*)((char*)_u -  sizeof(kn_dlist_node));
	if(u->dictionary){
		hash_map_destroy(u->dictionary,hash_destroyer);
	}
	free(u->stack);
	free(u);	
}

static uint64_t ut_hash_func(void *key){
	return burtle_hash((uint8_t*)key,strlen((char*)key),1);
}

static uint64_t ut_snd_hash_func(void *key){
	return burtle_hash((uint8_t*)key,strlen((char*)key),2);	
}

static int ut_key_cmp(void *key1,void*key2){
	return strncmp((char*)key1,(char*)key2,64);
} 


static struct uthread* uthread_create(void*stack,void(*fun)(void*),void *param)
{
	struct uthread* u = (struct uthread*)calloc(1,sizeof(*u));
	refobj_init(&u->refobj,uthread_destroy);
	getcontext(&(u->ucontext));		
	if(stack){	
		u->ucontext.uc_stack.ss_sp = stack;
		u->ucontext.uc_stack.ss_size = t_scheduler->stacksize;
		u->ucontext.uc_link = NULL;
		u->main_fun = fun;
		u->param = param;
		u->stack = stack;
		u->dictionary = hash_map_create(16,ut_hash_func,ut_key_cmp,ut_snd_hash_func);
		makecontext(&(u->ucontext),(void(*)())uthread_main_function,1,u);
	}	
	return u;
}

static int8_t less(struct heapele* _l,struct heapele* _r){
	struct uthread* ut_l = (struct uthread*)((char*)_l -  sizeof(kn_dlist_node) - sizeof(refobj));
	struct uthread* ut_r = (struct uthread*)((char*)_r -  sizeof(kn_dlist_node) - sizeof(refobj));
	return ut_l->timeout < ut_r->timeout ? 1:0;
}

static int add2ready(struct uthread* ut){
	if(ut->status == start || 
	   ut->status == blocking || 
	   ut->status == sleeping || 
	   ut->status == yield){
		if(ut->timeout){
			//remove from timer
			minheap_remove(t_scheduler->timer,&ut->heapnode);
		}
		if(0 == kn_dlist_push(&(t_scheduler->ready_list),(kn_dlist_node*)ut)){
			ut->status = ready;
			++t_scheduler->activecount;
			return 0;
		}
	} 
	return -1;
}

int     uscheduler_init(uint32_t stack_size){
	if(t_scheduler) return -1;
	t_scheduler = calloc(1,sizeof(*t_scheduler));
	stack_size = get_pow2(stack_size);
	if(stack_size < 8192) stack_size = 8192;
	t_scheduler->stacksize = stack_size;
	t_scheduler->ut_scheduler = uthread_create(NULL,NULL,NULL);
	kn_dlist_init(&(t_scheduler->ready_list));
	t_scheduler->timer = minheap_create(4096,less);
	return 0;
}

int uscheduler_clear(){
	if(t_scheduler && t_scheduler->current == NULL){
		struct uthread *next;
		while((next = (struct uthread *)kn_dlist_pop(&t_scheduler->ready_list))){
			refobj_dec(&next->refobj);
		};			
		while((next = (struct uthread *)minheap_popmin(t_scheduler->timer))){
			next = (struct uthread*)((char*)next - sizeof(refobj) - sizeof(kn_dlist_node));
			refobj_dec(&next->refobj);
		}
		refobj_dec(&t_scheduler->ut_scheduler->refobj);
		minheap_destroy(&t_scheduler->timer);
		free(t_scheduler);
		t_scheduler = NULL;
		return 0;
	}
	return -1;
}

uthread_t ut_spawn(void(*fun)(void*),void *param){
	uthread_t uident = {.identity=0,.ptr=0};
	if(!t_scheduler || !fun) return uident;
	void *stack = calloc(1,t_scheduler->stacksize);
	if(!stack)  return uident;
	struct uthread *ut = uthread_create(stack,fun,param);
	add2ready(ut);
	return make_ident(&ut->refobj);
}

int uschedule(){
	assert(t_scheduler);
	kn_dlist tmp;
	kn_dlist_init(&tmp);
	struct uthread *next;
	while((next = (struct uthread *)kn_dlist_pop(&t_scheduler->ready_list))){
		t_scheduler->current = next;
		next->status = running;
		uthread_switch(t_scheduler->ut_scheduler,next);
		t_scheduler->current = NULL;
		if(next->status == dead){
			refobj_dec(&next->refobj);
		}else if(next->status == yield){
			kn_dlist_push(&tmp,(kn_dlist_node*)next);
		}
	};
	//process timeout
	uint32_t now = kn_systemms();
	while((next = (struct uthread *)minheap_min(t_scheduler->timer))){
		next = (struct uthread*)((char*)next - sizeof(refobj) - sizeof(kn_dlist_node));
		if(next->timeout > now) break;
		minheap_popmin(t_scheduler->timer);
		add2ready(next);
	}
	while((next = (struct uthread *)kn_dlist_pop(&tmp))){
		add2ready(next);
	}
	return t_scheduler->activecount;
}

int ut_yield(){
	if(!t_scheduler || !t_scheduler->current)
		return -1;
	t_scheduler->current->status = yield;
	--t_scheduler->activecount;
	uthread_switch(t_scheduler->current,t_scheduler->ut_scheduler);
	return 0;
}

int ut_block(uint32_t ms){
	if(!t_scheduler || !t_scheduler->current)
		return -1;
	if(ms){
		t_scheduler->current->timeout = kn_systemms() + ms;
		minheap_insert(t_scheduler->timer,&t_scheduler->current->heapnode);		
	}
	if(t_scheduler->current->status != sleeping)
		t_scheduler->current->status = blocking;
	--t_scheduler->activecount;	
	uthread_switch(t_scheduler->current,t_scheduler->ut_scheduler);
	return 0;		
}

int ut_sleep(uint32_t ms){
	if(ms == 0) return ut_yield();
	t_scheduler->current->status = sleeping;
	return ut_block(ms);		
}  

uthread_t ut_getcurrent(){
	return make_ident(&t_scheduler->current->refobj);
}

static inline struct uthread *cast2uthread(uthread_t u){
	struct uthread *ut = (struct uthread*)cast2refobj(u);
	if(!ut) return NULL;
	return (struct uthread*)((char*)ut  - sizeof(kn_dlist_node));
}

int ut_wakeup(uthread_t u){
	struct uthread *ut =cast2uthread(u);
	if(!ut) return -1;
	add2ready(ut);
	refobj_dec(&ut->refobj);
	return 0;
} 


void *ut_dic_get(uthread_t u,const char *key){
	struct uthread *ut =cast2uthread(u);
	if(!ut) return NULL;
	struct ut_dic_node *node = (struct ut_dic_node*)hash_map_find(ut->dictionary,(void*)key);
	refobj_dec(&ut->refobj);
	if(node)
		return node->value;
	return NULL;
}

int   ut_dic_set(uthread_t u,const char *key,void *value,void (*value_destroyer)(void*)){
	if(strlen(key) +1 > MAX_KEY_SIZE) return -1;
	struct uthread *ut =cast2uthread(u);
	if(!ut) return -1;
	struct ut_dic_node *node = (struct ut_dic_node*)hash_map_find(ut->dictionary,(void*)key);
	if(node){
		//clear old
		if(node->value_destroyer && node->value){
			node->value_destroyer(node->value);	
		}
		node->value = value;
		node->value_destroyer = value_destroyer;		
	}else{
		node = calloc(1,sizeof(*node));
		node->_hash_node.key = node->_key;
		strncpy(node->_key,(char*)key,64);
		node->value = value;
		node->value_destroyer = value_destroyer;
		hash_map_insert(ut->dictionary,(hash_node*)node);		
	}
	refobj_dec(&ut->refobj);
	return 0;
}





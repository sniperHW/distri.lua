#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>
#include "kn_list.h"
#include "uthread/uthread.h"
#include "minheap.h"
#include "kn_common_define.h"
#include "kn_time.h"
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
   kn_list_node  node;
   refobj             refobj;	   
   struct heapele heapnode;	
   ucontext_t ucontext;
   uint32_t     timeout;	
   void *param;   		
   struct uthread* parent;
   uint8_t       status;
   void   *stack;
   void (*main_fun)(void*);
};

typedef struct {
	kn_list ready_list;
	struct uthread *current;
	struct uthread *ut_scheduler;
	uint32_t      stacksize;
	minheap_t  timer;
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
	if(u->parent)
		uthread_switch(u,u->parent);
}

void uthread_destroy(void *_u)
{
	struct uthread* u = (struct uthread*)((char*)_u -  sizeof(kn_list_node));
	free(u->stack);
	free(u);	
}

static struct uthread* uthread_create(struct uthread* parent,void*stack,void(*fun)(void*),void *param)
{
	struct uthread* u = (struct uthread*)calloc(1,sizeof(*u));
	refobj_init(&u->refobj,uthread_destroy);	
	if(stack){	
		getcontext(&(u->ucontext));
		u->ucontext.uc_stack.ss_sp = stack;
		u->ucontext.uc_stack.ss_size = t_scheduler->stacksize;
		u->ucontext.uc_link = NULL;
		u->parent = parent;
		u->main_fun = fun;
		u->param = param;
		u->stack = stack;
		makecontext(&(u->ucontext),(void(*)())uthread_main_function,1,u);
	}	
	return u;
}

static int8_t less(struct heapele* _l,struct heapele* _r){
	struct uthread* ut_l = (struct uthread*)((char*)_l -  sizeof(kn_list_node) - sizeof(refobj));
	struct uthread* ut_r = (struct uthread*)((char*)_r -  sizeof(kn_list_node) - sizeof(refobj));
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
		ut->status = ready;
		kn_list_pushback(&(t_scheduler->ready_list),(kn_list_node*)ut);
		return 0;
	} 
	return -1;
}

int     uscheduler_init(uint32_t stack_size){
	if(t_scheduler) return -1;
	t_scheduler = calloc(1,sizeof(*t_scheduler));
	stack_size = get_pow2(stack_size);
	if(stack_size < 4096) stack_size = 4096;
	t_scheduler->stacksize = stack_size;
	t_scheduler->ut_scheduler = uthread_create(NULL,NULL,NULL,NULL);
	kn_list_init(&(t_scheduler->ready_list));
	t_scheduler->timer = minheap_create(4096,less);
	return 0;
}

int uscheduler_clear(){
	if(t_scheduler && t_scheduler->current == NULL){
		struct uthread *next;
		while((next = (struct uthread *)kn_list_pop(&t_scheduler->ready_list))){
			refobj_dec(&next->refobj);
		};			
		while((next = (struct uthread *)minheap_popmin(t_scheduler->timer))){
			next = (struct uthread*)((char*)next - sizeof(refobj) - sizeof(kn_list_node));
			refobj_dec(&next->refobj);
		}
		refobj_dec(&t_scheduler->ut_scheduler->refobj);
		minheap_destroy(&t_scheduler->timer);
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
	struct uthread *ut = uthread_create(t_scheduler->ut_scheduler,stack,fun,param);
	add2ready(ut);
	return make_ident(&ut->refobj);
}

int uschedule(){
	if(!t_scheduler) return -1;
	kn_list tmp = {.size=0,.head=NULL,.tail=NULL};
	struct uthread *next;
	while((next = (struct uthread *)kn_list_pop(&t_scheduler->ready_list))){
		t_scheduler->current = next;
		next->status = running;
		uthread_switch(t_scheduler->ut_scheduler,next);
		t_scheduler->current = NULL;
		if(next->status == dead){
			refobj_dec(&next->refobj);
		}else if(next->status == yield){
			kn_list_pushback(&tmp,(kn_list_node*)next);
		}
	};
	//process timeout
	uint32_t now = kn_systemms();
	while((next = (struct uthread *)minheap_min(t_scheduler->timer))){
		next = (struct uthread*)((char*)next - sizeof(refobj) - sizeof(kn_list_node));
		if(next->timeout > now) break;
		minheap_popmin(t_scheduler->timer);
		add2ready(next);
	}
	while((next = (struct uthread *)kn_list_pop(&tmp))){
		add2ready(next);
	}
	return kn_list_size(&t_scheduler->ready_list);
}

int ut_yield(){
	if(!t_scheduler || !t_scheduler->current)
		return -1;
	t_scheduler->current->status = yield;
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

int ut_wakeup(uthread_t u){
	struct uthread *ut = (struct uthread*)cast2refobj(u);
	if(!ut) return -1;
	ut = (struct uthread*)((char*)ut  - sizeof(kn_list_node));
	add2ready(ut);
	refobj_dec(&ut->refobj);
	return 0;
} 




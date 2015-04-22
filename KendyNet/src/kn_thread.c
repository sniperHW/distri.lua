#include <stdlib.h>
#include "kn_thread.h"
#include "kn_thread_sync.h"

struct start_arg{
	void                  *arg;
	void                  *(*routine)(void*);
	uint8_t              running;
	kn_mutex_t      mtx;
	kn_condition_t cond;
};

kn_thread_t kn_create_thread(int32_t joinable)
{
	kn_thread_t t = calloc(1,sizeof(*t));
	t->joinable = joinable;
	return t;
}

void kn_thread_destroy(kn_thread_t t)
{
	free(t);
}

void* kn_thread_join(kn_thread_t t)
{
	void *result = 0;
	if(t->joinable)
		pthread_join(t->threadid,&result);
	return result;
}

void kn_thread_start(kn_thread_t t,kn_thread_routine r,void *arg)
{
	pthread_attr_t attr;
	if(!t)
		return;
	pthread_attr_init(&attr);
	if(t->joinable)
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	else
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&t->threadid,&attr,r,arg);
	pthread_attr_destroy(&attr);
}

static void *routine(void *_){
	struct start_arg *starg = (struct start_arg*)_;
	void *arg = starg->arg;
	void*(*routine)(void*) = starg->routine;
	kn_mutex_lock(starg->mtx);
	if(!starg->running){
		starg->running = 1;
		kn_condition_signal(starg->cond);
	}
	kn_mutex_unlock(starg->mtx);
	return routine(arg);
}

void kn_thread_start_wait(kn_thread_t t,kn_thread_routine r,void *arg)
{
	pthread_attr_t attr;
	if(!t)
		return;
	pthread_attr_init(&attr);
	if(t->joinable)
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	else
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	struct start_arg starg;

	starg.routine = r;
	starg.arg = arg;
	starg.running = 0;
	starg.mtx = kn_mutex_create();
	starg.cond = kn_condition_create();
	pthread_create(&t->threadid,&attr,routine,&starg);


	kn_mutex_lock(starg.mtx);
	while(!starg.running){
		kn_condition_wait(starg.cond,starg.mtx);
	}
	kn_mutex_unlock(starg.mtx);
	kn_mutex_destroy(starg.mtx);
	kn_condition_destroy(starg.cond);
	pthread_attr_destroy(&attr);
}


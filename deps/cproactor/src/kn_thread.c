#include <stdlib.h>
#include "kn_thread.h"


kn_thread_t kn_create_thread(int32_t joinable)
{
	kn_thread_t t = calloc(1,sizeof(*t));
	t->joinable = joinable;
	t->is_suspend = 0;
	t->mtx = kn_mutex_create();
	t->cond = kn_condition_create();
	return t;
}

void kn_thread_destroy(kn_thread_t t)
{
	kn_mutex_destroy(t->mtx);
	kn_condition_destroy(t->cond);
	free(t);
}

void* kn_thread_join(kn_thread_t t)
{
	void *result = 0;
	if(t->joinable)
		pthread_join(t->threadid,&result);
	return result;
}

void kn_thread_start_run(kn_thread_t t,kn_thread_routine r,void *arg)
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

void kn_thread_suspend(kn_thread_t t,int32_t ms)
{
	pthread_t self = pthread_self();
//#ifdef _WIN
//	if(self.p != t->threadid.p || self.x != t->threadid.x)
//		return;
//#else	
	if(self != t->threadid)
		return;//只能挂起自己
//#endif	
	kn_mutex_lock(t->mtx);
	if(0 >= ms)
	{
		t->is_suspend = 1;
		while(t->is_suspend)
		{
			kn_condition_wait(t->cond,t->mtx);
		}
	}
	else
	{
		t->is_suspend = 1;
		kn_condition_timedwait(t->cond,t->mtx,ms);
		t->is_suspend = 0;
	}
	kn_mutex_unlock(t->mtx);
}

void kn_thread_resume(kn_thread_t t)
{
	pthread_t self = pthread_self();
//#ifdef _WIN
//	if(self.p != t->threadid.p || self.x != t->threadid.x)
//		return;
//#else	
	if(self == t->threadid)
		return;//只能有其它线程resume
//#endif
	kn_mutex_lock(t->mtx);
	if(t->is_suspend)
	{
		t->is_suspend = 0;
        kn_mutex_unlock(t->mtx);
		kn_condition_signal(t->cond);
        return;
    }
    kn_mutex_unlock(t->mtx);
}

typedef struct kn_thread_arg
{
	kn_thread_t _thread;
	void *custom_arg;
	kn_thread_routine custom_routine;
}kn_thread_arg;


static void *routine(void *arg)
{
	kn_thread_arg *_arg = (kn_thread_arg *)arg;
	kn_thread_t t = _arg->_thread;
	void *custom_arg = _arg->custom_arg;
	kn_thread_routine custom_routine = _arg->custom_routine;
	custom_routine(custom_arg);
	kn_thread_destroy(t);
	free(_arg);
	return 0;
	
}

void  kn_thread_run(kn_thread_routine r,void *arg)
{
	struct kn_thread_arg *_thread_arg = malloc(sizeof(*_thread_arg));
	_thread_arg->custom_routine = r;
	_thread_arg->custom_arg = arg;
	_thread_arg->_thread = kn_create_thread(0);//not joinable
	kn_thread_start_run(_thread_arg->_thread,routine,_thread_arg);
}

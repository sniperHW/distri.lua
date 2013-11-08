#include <stdlib.h>
#include "thread.h"
#include "sync.h"

typedef struct thread
{
	pthread_t threadid;
	volatile int32_t is_suspend;
	int32_t joinable;
	mutex_t mtx;
	condition_t  cond;
}thread;


thread_t create_thread(int32_t joinable)
{
	thread_t t = malloc(sizeof(thread));
	t->joinable = joinable;
	t->is_suspend = 0;
	t->mtx = mutex_create();
	t->cond = condition_create();
	return t;
}

void destroy_thread(thread_t *t)
{
	mutex_destroy(&(*t)->mtx);
	condition_destroy(&(*t)->cond);
	free(*t);
	*t = 0;
}

void* thread_join(thread_t t)
{
	void *result = 0;
	if(t->joinable)
		pthread_join(t->threadid,&result);
	return result;
}

void thread_start_run(thread_t t,thread_routine r,void *arg)
{
	if(!t)
		return;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(t->joinable)
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	else
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&t->threadid,&attr,r,arg);
	pthread_attr_destroy(&attr);
}

void thread_suspend(thread_t t,int32_t ms)
{
	pthread_t self = pthread_self();
#ifdef _WIN
	if(self.p != t->threadid.p || self.x != t->threadid.x)
		return;
#else	
	if(self != t->threadid)
		return;//只能挂起自己
#endif	
	mutex_lock(t->mtx);
	if(0 >= ms)
	{
		t->is_suspend = 1;
		while(t->is_suspend)
		{
			condition_wait(t->cond,t->mtx);
		}
	}
	else
	{
		t->is_suspend = 1;
		condition_timedwait(t->cond,t->mtx,ms);
		t->is_suspend = 0;
	}
	mutex_unlock(t->mtx);
}

void thread_resume(thread_t t)
{
	pthread_t self = pthread_self();
#ifdef _WIN
	if(self.p != t->threadid.p || self.x != t->threadid.x)
		return;
#else	
	if(self == t->threadid)
		return;//只能有其它线程resume
#endif
	mutex_lock(t->mtx);
	if(t->is_suspend)
	{
		t->is_suspend = 0;
        mutex_unlock(t->mtx);
		condition_signal(t->cond);
        return;
    }
    mutex_unlock(t->mtx);
}

struct thread_arg
{
	thread_t _thread;
	void *custom_arg;
	thread_routine custom_routine;
};


static void *routine(void *arg)
{
	struct thread_arg *_arg = (struct thread_arg *)arg;
	thread_t t = _arg->_thread;
	void *custom_arg = _arg->custom_arg;
	thread_routine custom_routine = _arg->custom_routine;
	custom_routine(custom_arg);
	destroy_thread(&t);
	free(_arg);
	return 0;
	
}

void  thread_run(thread_routine r,void *arg)
{
	struct thread_arg *_thread_arg = malloc(sizeof(*_thread_arg));
	_thread_arg->custom_routine = r;
	_thread_arg->custom_arg = arg;
	_thread_arg->_thread = create_thread(0);//not joinable
	thread_start_run(_thread_arg->_thread,routine,_thread_arg);
}

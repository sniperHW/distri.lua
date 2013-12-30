#include "uthread.h"
#include <stdlib.h>
#include <pthread.h>
#include "llist.h"

#ifdef _USE_UCONTEXT

#include "uthread_ucontext.h"

#else


struct uthread
{
    /*for x64,0:rsp,1:rbp,2:rax,3:rbx,4:r12,5:r13,6:r14,7:r15
    * for x32,0:esp,1:ebp,2:eax,3:ebx,4:ecx,5:edx,6:edi,7:esi
	*/
	void* reg[8];
	void *para;
	uthread_t parent;
	void*(*main_fun)(void*);
	void *stack;
	int32_t ssize;
	int8_t first_run;
};

void __attribute__((regparm(1))) uthread_main_function(void *arg)
{
	volatile uthread_t u = (uthread_t)arg;
	void *ret = u->main_fun(u->para);
	if(u->parent)
		uthread_switch(u,u->parent,ret);
	else
		exit(0);
}

uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*))
{
	uthread_t u = (uthread_t)calloc(1,sizeof(*u));
	u->parent = parent;
	u->main_fun = fun;
	u->stack = stack;
	u->ssize = stack_size;
	if(stack)
	{
		u->reg[0] = (void*)((char*)stack+stack_size-64);
		u->reg[1] = (void*)((char*)stack+stack_size-64);
	}
	if(u->main_fun)
		u->first_run = 1;
	return u;
}

void uthread_destroy(uthread_t *u)
{
	free(*u);
	*u = NULL;
}
#ifdef _X64
#include "uthread_64.h"
#else
#include "uthread_32.h"
#endif

#endif


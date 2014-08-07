#include <ucontext.h>

struct uthread
{
   ucontext_t ucontext;	
   void *para;   		
   uthread_t  parent;
   void* (*main_fun)(void*);
};

void uthread_main_function(void *arg)
{
	uthread_t u = (uthread_t)arg;
	void *ret = u->main_fun(u->para);
	if(u->parent)
		uthread_switch(u,u->parent,ret);
}

uthread_t uthread_create(uthread_t parent,void*stack,uint32_t stack_size,void*(*fun)(void*))
{
	uthread_t u = (uthread_t)calloc(1,sizeof(*u));	
	if(stack && stack_size && fun)
	{
		getcontext(&(u->ucontext));
		u->ucontext.uc_stack.ss_sp = stack;
		u->ucontext.uc_stack.ss_size = stack_size;
		u->ucontext.uc_link = NULL;
		u->parent = parent;
		u->main_fun = fun;
		makecontext(&(u->ucontext),(void(*)())uthread_main_function,1,u);	
	}
	return u;
}

void uthread_destroy(uthread_t *u)
{
	free(*u);
	*u = NULL;	
}

void* __attribute__((regparm(3)))uthread_switch(uthread_t from,uthread_t to,void *para)
{
    if(!from) return NULL;
	to->para = para;
	swapcontext(&(from->ucontext),&(to->ucontext));
	return from->para;
}

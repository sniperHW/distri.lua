#include "uthread.h"
#include <stdlib.h>
#include <pthread.h>
#include "llist.h"

#ifdef _DEBUG
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
    if(!from) return NULL;
	to->para = para;
	void *rsp,*rbp,*rax,*rbx,*r12,*r13,*r14,*r15;
	//save current registers
	//the order is important
	 __asm__ volatile(
		"movq %%rax,%2\t\n"
		"movq %%rbx,%3\t\n"
		"movq %%r12,%4\t\n"
		"movq %%r13,%5\t\n"
		"movq %%r14,%6\t\n"
		"movq %%r15,%7\t\n"
		"movq %%rbp,%1\t\n"
		"movq %%rsp,%0\t\n"
		:
		:"m"(rsp),"m"(rbp),"m"(rax),"m"(rbx),"m"(r12),"m"(r13),"m"(r14),"m"(r15)
	);
	from->reg[0] = rsp;
	from->reg[1] = rbp;
	from->reg[2] = rax;
	from->reg[3] = rbx;
	from->reg[4] = r12;
	from->reg[5] = r13;
	from->reg[6] = r14;
	from->reg[7] = r15;
	if(to->first_run)
	{
       if(!to->parent) to->parent = from;
	   to->first_run = 0;
	   rsp = to->reg[0];
	   //use rcx to pass arg
		__asm__ volatile (
			"movq %1,%%rcx\t\n"
			"movq %0,%%rbp\t\n"
			"movq %%rbp,%%rsp\t\n"
			"movq %%rcx,%1\t\n"
			:
			:"m"(rsp),"m"(to)
		);
	   uthread_main_function(to);
	}
	else
	{
		rsp = to->reg[0];
		rbp = to->reg[1];
		rax = to->reg[2];
		rbx = to->reg[3];
		r12 = to->reg[4];
		r13 = to->reg[5];
		r14 = to->reg[6];
		r15 = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movq %2,%%rax\t\n"
			"movq %3,%%rbx\t\n"
			"movq %4,%%r12\t\n"
			"movq %5,%%r13\t\n"
			"movq %6,%%r14\t\n"
			"movq %7,%%r15\t\n"
			"movq %1,%%rbp\t\n"
			"movq %0,%%rsp\t\n"
			:
			:"m"(rsp),"m"(rbp),"m"(rax),"m"(rbx),"m"(r12),"m"(r13),"m"(r14),"m"(r15)
		);
	}
	return from->para;
}
#else
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
    if(!from) return NULL;
	to->para = para;
	void *rsp,*rbp,*r14,*r15;
	//save current registers
	//the order is important
	 __asm__ volatile(
		"movq %%rax,%2\t\n"
		"movq %%rbx,%3\t\n"
		"movq %%r12,%4\t\n"
		"movq %%r13,%5\t\n"
		"movq %%r14,%6\t\n"
		"movq %%r15,%7\t\n"
		"movq %%rbp,%1\t\n"
		"movq %%rsp,%0\t\n"
		:
		:"m"(from->reg[0]),"m"(from->reg[1]),"m"(from->reg[2]),"m"(from->reg[3])
		,"m"(from->reg[4]),"m"(from->reg[5]),"m"(from->reg[6]),"m"(from->reg[7])
	);
	if(to->first_run)
	{
       if(!to->parent) to->parent = from;
	   to->first_run = 0;
	   //change stack
	   //the order is important
		__asm__ volatile (
			"movq %0,%%rbp\t\n"
			"movq %%rbp,%%rsp\t\n"
			:
			:"m"(to->reg[0])
		);
	   uthread_main_function((void*)to);
	}
	else
	{
		rsp = to->reg[0];
		rbp = to->reg[1];
		r14 = to->reg[6];
		r15 = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movq %2,%%rax\t\n"
			"movq %3,%%rbx\t\n"
			"movq %4,%%r12\t\n"
			"movq %5,%%r13\t\n"
			"movq %6,%%r14\t\n"
			"movq %7,%%r15\t\n"
			"movq %1,%%rbp\t\n"
			"movq %0,%%rsp\t\n"
			:
			:"m"(rsp),"m"(rbp),"m"(to->reg[2]),"m"(to->reg[3])
			,"m"(to->reg[4]),"m"(to->reg[5]),"m"(r14),"m"(r15)
		);
	}
	return from->para;
}
#endif

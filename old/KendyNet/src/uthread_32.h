#include "uthread.h"
#include <stdlib.h>
#include <pthread.h>
#include "llist.h"

#ifdef _DEBUG
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
	if(!from)
		return NULL;
	to->para = para;
	void *esp,*ebp,*eax,*ebx,*ecx,*edx,*edi,*esi;
	//save current registers
	//the order is important
	 __asm__ volatile(
		"movl %%eax,%2\t\n"
		"movl %%ebx,%3\t\n"
		"movl %%ecx,%4\t\n"
		"movl %%edx,%5\t\n"
		"movl %%edi,%6\t\n"
		"movl %%esi,%7\t\n"
		"movl %%ebp,%1\t\n"
		"movl %%esp,%0\t\n"
		:
		:"m"(esp),"m"(ebp),"m"(eax),"m"(ebx),"m"(ecx),"m"(edx),"m"(edi),"m"(esi)
	);
	from->reg[0] = esp;
	from->reg[1] = ebp;
	from->reg[2] = eax;
	from->reg[3] = ebx;
	from->reg[4] = ecx;
	from->reg[5] = edx;
	from->reg[6] = edi;
	from->reg[7] = esi;
	if(to->first_run)
	{
	   if(!to->parent)
            to->parent = from;
	   to->first_run = 0;
	   esp = to->reg[0];
		__asm__ volatile (
			"movl %1,%%eax\t\n"
			"movl %0,%%ebp\t\n"
			"movl %%ebp,%%esp\t\n"
			"movl %%eax,%1\t\n"
			:
			:"m"(esp),"m"(to)
		);
	   uthread_main_function(to);
	}
	else
	{
		esp = to->reg[0];
		ebp = to->reg[1];
		eax = to->reg[2];
		ebx = to->reg[3];
		ecx = to->reg[4];
		edx = to->reg[5];
		edi = to->reg[6];
		esi = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movl %2,%%eax\t\n"
			"movl %3,%%ebx\t\n"
			"movl %4,%%ecx\t\n"
			"movl %5,%%edx\t\n"
			"movl %6,%%edi\t\n"
			"movl %7,%%esi\t\n"
			"movl %1,%%ebp\t\n"
			"movl %0,%%esp\t\n"
			:
			:"m"(esp),"m"(ebp),"m"(eax),"m"(ebx),"m"(ecx),"m"(edx),"m"(edi),"m"(esi)
		);
	}
	return from->para;
}
#else
void* __attribute__((regparm(3))) uthread_switch(uthread_t from,uthread_t to,void *para)
{
    if(!from) return NULL;
	to->para = para;
	void *esp,*ebp,*edi,*esi;
	//save current registers
	//the order is important
	 __asm__ volatile(
		"movl %%eax,%2\t\n"
		"movl %%ebx,%3\t\n"
		"movl %%ecx,%4\t\n"
		"movl %%edx,%5\t\n"
		"movl %%edi,%6\t\n"
		"movl %%esi,%7\t\n"
		"movl %%ebp,%1\t\n"
		"movl %%esp,%0\t\n"
		:
		:"m"(from->reg[0]),"m"(from->reg[1]),"m"(from->reg[2]),"m"(from->reg[3])
		,"m"(from->reg[4]),"m"(from->reg[5]),"m"(from->reg[6]),"m"(from->reg[7])
	);
	if(to->first_run){
       if(!to->parent) to->parent = from;
	   to->first_run = 0;
	   //change stack
	   //the order is important
		__asm__ volatile (
			"movl %0,%%ebp\t\n"
			"movl %%ebp,%%esp\t\n"
			:
			:"m"(to->reg[0])
		);
	   uthread_main_function((void*)to);
	}
	else
	{
		esp = to->reg[0];
		ebp = to->reg[1];
		edi = to->reg[6];
		esi = to->reg[7];
		//the order is important
		__asm__ volatile (
			"movl %2,%%eax\t\n"
			"movl %3,%%ebx\t\n"
			"movl %4,%%ecx\t\n"
			"movl %5,%%edx\t\n"
			"movl %6,%%edi\t\n"
			"movl %7,%%esi\t\n"
			"movl %1,%%ebp\t\n"
			"movl %0,%%esp\t\n"
			:
			:"m"(esp),"m"(ebp),"m"(to->reg[2]),"m"(to->reg[3])
			,"m"(to->reg[4]),"m"(to->reg[5]),"m"(edi),"m"(esi)
		);
	}
	return from->para;
}
#endif

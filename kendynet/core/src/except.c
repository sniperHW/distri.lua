#include "except.h"
#include <stdlib.h>

struct exception_frame *exception_stack = 0;

void exception_throw(int32_t code,const char *file,int32_t line)
{
	if(exception_stack)
	{
		
		exception_stack->exception = code;
		exception_stack->file = file;
		exception_stack->line = line;
		struct exception_frame *frame = exception_stack;
		exception_stack = frame->pre;
		longjmp(frame->jumpbuffer,code);
	}
	else
	{
		printf("unsolved exception %d,file:%s line:%d\n",code,file,line);
	}
}

#include "except.h"
#include <stdlib.h>
#include "exception.h"

pthread_key_t g_exception_key;
pthread_once_t g_exception_key_once = PTHREAD_ONCE_INIT;


static void delete_thd_exstack(void  *arg)
{
    free(arg);
}

void exception_once_routine(){
    pthread_key_create(&g_exception_key,delete_thd_exstack);
}

void exception_throw(int32_t code,const char *file,const char *func,int32_t line)
{
    struct exception_frame *frame = expstack_top();
    if(frame)
    {
        frame->exception = code;
        frame->line = line;
        frame->is_process = 0;
        longjmp(frame->jumpbuffer,code);
    }
	else
	{
		printf("unsolved exception %d,file:%s line:%d\n",code,file,line);
	}
}


const char* exceptions[MAX_EXCEPTION] = {
    "invaile_excepton_no",
    "except_alloc_failed",
    "except_list_empty",
    "testexception1",
    "testexception2",
    "testexception3",
    NULL,
};

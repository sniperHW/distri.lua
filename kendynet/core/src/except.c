#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
//#include <ucontext.h>
//#include <dlfcn.h>
#include <execinfo.h>
#include "exception.h"
#include "except.h"

pthread_key_t g_exception_key;
pthread_once_t g_exception_key_once = PTHREAD_ONCE_INIT;


static void delete_thd_exstack(void  *arg)
{
    free(arg);
}

static void signal_segv(int signum,siginfo_t* info, void*ptr){
    THROW(segmentation_fault);
    return;
}

int setup_sigsegv(){
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_segv;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &action, NULL) < 0) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

void exception_once_routine(){
    setup_sigsegv();
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
        void *bt[20];
        char **strings;
        size_t sz;
        int i;
        sz = backtrace(bt, 20);
        strings = backtrace_symbols(bt, sz);
        for(i = 0; i < sz; ++i){
            if(strstr(strings[i],"exception_throw+")){
                if(code == segmentation_fault) i+=2;
                continue;
            }
            struct callstack_frame *call_frame = calloc(1,sizeof(*call_frame));
            snprintf(call_frame->info,4096,"%s\n",strings[i]);
            LLIST_PUSH_BACK(&frame->call_stack,call_frame);
            if(strstr(strings[i],"main+"))
                break;
        }
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
    "segmentation_fault",
    "testexception1",
    "testexception2",
    "testexception3",
    NULL,
};

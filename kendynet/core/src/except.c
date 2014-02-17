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
	struct exception_perthd_st *epst = (struct exception_perthd_st*)arg;
	while(!LLIST_IS_EMPTY(&epst->csf_pool))
        free(LLIST_POP(void*,&epst->csf_pool));
    free(arg);
}

int setup_sigsegv();
static void signal_segv(int signum,siginfo_t* info, void*ptr){
    THROW(except_segv_fault);
    return;
}
   
int setup_sigsegv(){
    printf("setup_sigsegv\n");
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    //sigaddset(&action.sa_mask,SIGINT);
    action.sa_sigaction = signal_segv;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &action, NULL) < 0) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

static void signal_sigbus(int signum,siginfo_t* info, void*ptr){
    THROW(except_sigbus);
    return;
}

int setup_sigbus(){
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_sigbus;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGBUS, &action, NULL) < 0) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

static void signal_sigfpe(int signum,siginfo_t* info, void*ptr){
    THROW(except_arith);
    return;
}

int setup_sigfpe(){
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_sigfpe;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGFPE, &action, NULL) < 0) {
        perror("sigaction");
        return 0;
    }
    return 1;
}

void exception_once_routine(){
    pthread_key_create(&g_exception_key,delete_thd_exstack);
    setup_sigsegv();
    setup_sigbus();
    setup_sigfpe();
}


static inline struct callstack_frame * get_csf(struct llist *pool)
{
	if(LLIST_IS_EMPTY(pool))
	{
		int32_t i = 0;
		for( ; i < 256; ++i){
			struct callstack_frame *call_frame = calloc(1,sizeof(*call_frame));
			LLIST_PUSH_FRONT(pool,(lnode*)call_frame);
		}
	}
	return LLIST_POP(struct callstack_frame*,pool);
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
        struct exception_perthd_st *epst = (struct exception_perthd_st *)pthread_getspecific(g_exception_key);
		for(i = 0; i < sz; ++i){
            if(strstr(strings[i],"exception_throw+")){
                if(code == except_segv_fault ||
                   code == except_sigbus     ||
                   code == except_arith) i+=2;
                continue;
            }
            struct callstack_frame *call_frame = get_csf(&epst->csf_pool);
            snprintf(call_frame->info,256,"%s\n",strings[i]);
            LLIST_PUSH_BACK(&frame->call_stack,call_frame);
            if(strstr(strings[i],"main+"))
                break;
        }
        free(strings);
        int sig = 0;
        if(code == except_segv_fault) sig = SIGSEGV;
        else if(code == except_sigbus) sig = SIGBUS;
        else if(code == except_arith) sig = SIGFPE;  
        siglongjmp(frame->jumpbuffer,sig);
    }
	else
	{
		printf("unsolved exception %d,file:%s line:%d\n",code,file,line);
	}
}


const char* exceptions[MAX_EXCEPTION] = {
    "except_invaild_num",
    "except_alloc_failed",
    "except_list_empty",
    "except_segv_fault",
    "except_sigbus",
    "except_arith",
    "testexception3",
    NULL,
};

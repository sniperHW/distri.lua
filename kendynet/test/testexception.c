//#define _GNU_SOURCE
#include <stdio.h>
#include "core/except.h"
#include "core/exception.h"
#include "core/thread.h"
#include <signal.h>
#include "core/systime.h"
/*
#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif


static void signal_segv(int signum, siginfo_t* info, void*ptr) {
        static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};

        struct exception_frame *frame = expstack_top();
        if(frame){
            size_t i;
            ucontext_t *ucontext = (ucontext_t*)ptr;
#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
            Dl_info dlinfo;
            void **bp = 0;
            void *ip = 0;
#else
            void *bt[20];
            char **strings;
            size_t sz;
#endif
#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
# if defined(SIGSEGV_STACK_IA64)
            ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
            bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
# elif defined(SIGSEGV_STACK_X86)
            ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
            bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
# endif
           // fprintf(stderr, "Stack trace:\n");
            while(bp && ip) {
                    if(!dladdr(ip, &dlinfo))
                            break;
                    const char *symname = dlinfo.dli_sname;
                    struct callstack_frame *call_frame = calloc(1,sizeof(*call_frame));
                    snprintf(call_frame->info,4096,"%p %s+%u\n",
                                    ip,
                                    symname,
                                    (unsigned)(ip - dlinfo.dli_saddr));
                    LLIST_PUSH_BACK(&frame->call_stack,call_frame);
                    if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
                            break;
                    ip = bp[1];
                    bp = (void**)bp[0];
            }
#else
            fprintf(stderr, "Stack trace (non-dedicated):\n");
            sz = backtrace(bt, 20);
            strings = backtrace_symbols(bt, sz);
            for(i = 0; i < sz; ++i)
                    fprintf(stderr, "%s\n", strings[i]);
#endif
           // fprintf(stderr, "End of stack trace\n");
            THROW(testexception1);
            return;
        }else
            abort();
}
*/

void func1()
{
    int *ptr = NULL;
    *ptr = 10;
}

void entry1()
{
    TRY{
        func1();
    }CATCH(except_segv_fault)
    {
        printf("catch:%s stack below\n",exception_description(EXPNO));
        PRINT_CALL_STACK;
    }CATCH_ALL
    {
        printf("catch all: %s\n",exception_description(EXPNO));
    }ENDTRY;
    
    return;
}

void func2()
{
    printf("func2\n");
    int *a = NULL;
    printf("%d\n",*a);
    
    //int a = 10;
    //printf("%d\n",1/0);
    printf("func2 end\n");
}

void entry2()
{
    TRY{
        func2();
    }CATCH(except_arith)
    {
        printf("catch:%s stack below\n",exception_description(EXPNO));
        PRINT_CALL_STACK;
    }CATCH_ALL
    {
        printf("catch all: %s\n",exception_description(EXPNO));
    }ENDTRY;
    
    return;
}

void func3()
{
    THROW(testexception3);
}

void *Routine1(void *arg)
{
    TRY{
        func3();
    }CATCH_ALL
    {
        printf("catch all: %s\n",exception_description(EXPNO));
        PRINT_CALL_STACK;
    }ENDTRY;
    
    return NULL;
}

static volatile int8_t stop = 0;

static void stop_handler(int signo){
    printf("stop_handler\n");
    stop = 1;
}

void setup_signal_handler()
{
    struct sigaction act;
    bzero(&act, sizeof(act));
    act.sa_handler = stop_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}


int main()
{
    setup_signal_handler();
    //entry1();
    entry2();
    printf("here\n");
    //while(!stop)
    //   sleepms(100);
    //setup_sigsegv();
    entry2();
    //thread_t t1 = create_thread(0);
    //thread_start_run(t1,Routine1,NULL);
    getchar();
    printf("main end\n");
    return 0;
}

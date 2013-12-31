//#define _GNU_SOURCE
#include <stdio.h>
#include "core/except.h"
#include "core/exception.h"

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
    //THROW(testexception1);
    return ;
}

void func2()
{
    return;
}

void entry()
{
    TRY{
        func1();
        func2();
    }CATCH(segmentation_fault)
    {
        printf("catch:%s stack below\n",exception_description(EXPNO));
        PRINT_CALL_STACK;
    }CATCH_ALL
    {
        printf("catch all: %s\n",exception_description(EXPNO));
    }
    ENDTRY;
    return;
}

int main()
{
    entry();
    printf("main end\n");
    return 0;
}

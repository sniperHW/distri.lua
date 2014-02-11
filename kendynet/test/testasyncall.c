#include <stdio.h>
#include <stdlib.h>
#include "asynnet/msgdisp.h"
#include <stdint.h>
#include "testcommon.h"
#include "asynnet/asyncall.h"

static void *service_main(void *ud){
    msgdisp_t disp = (msgdisp_t)ud;
    while(!stop){
        //sleepms(1000);
        msg_loop(disp,50);
    }
    return NULL;
}

struct testcall_context{
    asyncall_context base_context;
    int32_t c;
    msgdisp_t self;
    msgdisp_t processer;
};


void test_asyncall(asyncall_context_t context,void **param)
{
    printf("%d\n",(int32_t)param[0]);
    ASYNRETURN(context,NULL);
}

void test_result(struct asyncall_context *context,void *result)
{
    struct testcall_context *my_context = (struct testcall_context*)context;
    if(my_context->c++ < 1000)
        ASYNCALL1(my_context->self,my_context->processer,test_asyncall,context,test_result,my_context->c);
}




int main(int argc,char **argv)
{
    setup_signal_handler();
    msgdisp_t disp1 = new_msgdisp(NULL,0);

    msgdisp_t disp2 = new_msgdisp(NULL,0);
    thread_t service2 = create_thread(THREAD_JOINABLE);    

    thread_start_run(service2,service_main,(void*)disp2);    
    
    //发出第一个异步调用
    struct testcall_context tcontext;
    tcontext.c = 0;
    tcontext.self = disp1;
    tcontext.processer = disp2;
    ASYNCALL1(disp1,disp2,test_asyncall,&tcontext,test_result,1);
    while(!stop){
        msg_loop(disp1,50);
    }

    thread_join(service2);
    printf("end\n");
    return 0;
}
//#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
//#include "kn_thread.h"
#include "kn_time.h"
#include "kn_ringque.h"


DECLARE_RINGQUE_N(ringN_int,int);
ringN_int mq;

void *producer(void *arg)
{
	printf("producer begin\n");
    for(;;){
        int j = 0;
        for(; j < 5;++j)
        {
            int i = 0;
            for(; i < 10000000; ++i)
                ringN_int_push(mq,1);
        }
    }
    printf("producer end\n");
    return NULL;
}

uint64_t count = 0;

void *comsumer(void *arg)
{
	printf("comsumer begin\n");
	for( ; ; )
	{
		int n = ringN_int_pop(mq);
		++count;
	}
    printf("comsumer end\n");
    return NULL;
}

int main()
{	
	mq = new_ringN_int(65536);//new_ringque_N(65536);
	
	kn_thread_start_run(kn_create_thread(0),comsumer,NULL);
	kn_thread_start_run(kn_create_thread(0),comsumer,NULL);
	kn_thread_start_run(kn_create_thread(0),producer,NULL);
	kn_thread_start_run(kn_create_thread(0),producer,NULL);

	uint32_t tick = kn_systemms();
	for(;;){
		uint32_t now = kn_systemms();
		if(now - tick > 1000)
		{
			printf("recv:%d\n",(uint32_t)((count*1000)/(now-tick)));
			tick = now;
			count = 0;
		}
		kn_sleepms(1);
	}
	getchar();
	return 0;
}


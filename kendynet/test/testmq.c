#include <stdio.h>
#include <stdlib.h>
#include "core/KendyNet.h"
#include "core/thread.h"
#include "core/atomic.h"
#include "core/SysTime.h"
#include "core/msg_que.h"

list_node *node_list1[5];
list_node *node_list2[5];
msgque_t mq1;

void *Routine1(void *arg)
{
    for(;;){
        int j = 0;
        for(; j < 5;++j)
        {
            int i = 0;
            for(; i < 10000000; ++i)
            {
                msgque_put(mq1,&node_list2[j][i]);
            }
            sleepms(500);
        }
    }
	close_msgque(mq1);
	printf("Routine3 end\n");
}

void *Routine2(void *arg)
{
	uint64_t count = 0;
	uint64_t total_count = 0;
	uint32_t tick = GetSystemMs();
	for( ; ; )
	{
		list_node *n;
		if(0 != msgque_get(mq1,&n,50))
            break;
		if(n)
		{
			++count;
			++total_count;
		}
		uint32_t now = GetSystemMs();
		if(now - tick > 1000)
		{
			printf("recv:%d\n",(count*1000)/(now-tick));
			tick = now;
			count = 0;
		}
	}
	printf("Routine2 end\n");
}


int main()
{
	int i = 0;
	for( ; i < 5; ++i)
	{
		node_list1[i] = calloc(10000000,sizeof(list_node));
		node_list2[i] = calloc(10000000,sizeof(list_node));
	}
	mq1 = msgque_acquire(new_msgque(1024,NULL));
	thread_t t1 = create_thread(0);
	thread_start_run(t1,Routine1,NULL);

	thread_t t2 = create_thread(0);
	thread_start_run(t2,Routine2,NULL);
	getchar();
	msgque_release(mq1);
	return 0;
}


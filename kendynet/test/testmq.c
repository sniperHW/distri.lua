#include <stdio.h>
#include <stdlib.h>
#include "core/kendynet.h"
#include "core/thread.h"
#include "core/atomic.h"
#include "core/systime.h"
#include "core/msgque.h"

list_node *node_list1[5];
list_node *node_list2[5];
list_node *node_list3[5];
msgque_t mq1;

void *Routine1(void *arg)
{
	msgque_open_write(mq1);
    for(;;){
        int j = 0;
        for(; j < 5;++j)
        {
            int i = 0;
            for(; i < 10000000; ++i)
            {
                msgque_put(mq1,&node_list1[j][i]);
            }
            msgque_flush();
            sleepms(200);
        }
    }
    printf("Routine1 end\n");
    return NULL;
}

void *Routine2(void *arg)
{
	msgque_open_write(mq1);
    for(;;){
        int j = 0;
        for(; j < 5;++j)
        {
            int i = 0;
            for(; i < 10000000; ++i)
            {
                msgque_put(mq1,&node_list2[j][i]);
            }
            msgque_flush();
            sleepms(200);
        }
    }
    printf("Routine2 end\n");
    return NULL;
}

void *Routine3(void *arg)
{
	msgque_open_write(mq1);
	for(;;){
		int j = 0;
		for(; j < 5;++j)
		{
			int i = 0;
			for(; i < 10000000; ++i)
			{
				msgque_put(mq1,&node_list3[j][i]);
			}
			msgque_flush();
			sleepms(200);
		}
	}
	printf("Routine3 end\n");
	return NULL;
}

void *Routine4(void *arg)
{
	msgque_open_read(mq1);
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
			printf("recv:%lld\n",(count*1000)/(now-tick));
			tick = now;
			count = 0;
		}
	}
    printf("Routine4 end\n");
    return NULL;
}

int main()
{
	/*int i = 0;
	for( ; i < 5; ++i)
	{
		node_list1[i] = calloc(10000000,sizeof(list_node));
		node_list2[i] = calloc(10000000,sizeof(list_node));
		node_list3[i] = calloc(10000000,sizeof(list_node));
	}*/
	mq1 = new_msgque(1024,NULL);
    //thread_t t4 = create_thread(0);
    //thread_start_run(t4,Routine4,NULL);
	
    /*thread_t t1 = create_thread(0);
	thread_start_run(t1,Routine1,NULL);

    thread_t t2 = create_thread(0);
    thread_start_run(t2,Routine2,NULL);

	thread_t t3 = create_thread(0);
	thread_start_run(t3,Routine3,NULL);
	*/
	getchar();
	return 0;
}


#include "kendynet.h"
#include "kn_time.h"
#include "kn_thread.h"
#include "kn_msgque.h"

volatile int count = 0;

kn_msgque_t  msgque;

void *routine1(void *arg)
{
	printf("routine1\n");
	int i = 0;
	kn_msgque_writer_t writer = kn_open_writer(msgque);
	while(1){
		for(i = 0; i < 10000000; ++i){
			kn_msgque_write(writer,(void*)1,NULL);
		}
		kn_msgque_flush(writer);
		kn_sleepms(1);
	}
    return NULL;
}

void *routine2(void *arg)
{
	printf("routine2\n");
	kn_msgque_reader_t reader = kn_open_reader(msgque);
	void *msg;
	while(1){
		kn_msgque_read(reader,&msg,-1);
		if(msg) ++count;
	}
    return NULL;
}

int main(){

	msgque = kn_new_msgque(32);
	kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_start_run(t1,routine1,NULL);
	kn_thread_start_run(t2,routine2,NULL);		
 	uint64_t tick,now;
    tick = now = kn_systemms64();	
	while(1){
		kn_sleepms(100);
        now = kn_systemms64();
		if(now - tick > 1000)
		{
            uint32_t elapse = (uint32_t)(now-tick);
			printf("count:%d\n",count/elapse*1000);
			tick = now;
			count = 0;
		}		
	}	
	return 0;
}

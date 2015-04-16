#include "kendynet.h"
#include "kn_time.h"
#include "kn_thread.h"
#include "kn_msgque.h"
#include "kn_atomic.h"
volatile int count = 0;
volatile uint32_t c1 = 0;
volatile uint32_t c2 = 0;

kn_msgque_t  msgque;

void *producer1(void *arg)
{
	printf("producer1\n");
	int i = 0;
	kn_msgque_writer_t writer = kn_open_writer(msgque);
	while(1){
		for(i = 0; i < 10000000; ++i){
			kn_msgque_write(writer,(void*)1,NULL);
			ATOMIC_INCREASE(&c1);
		}
		kn_msgque_flush(writer);
		while(c1 > 0){
			FENCE();
			kn_sleepms(0);
		}
	}
    return NULL;
}

void *producer2(void *arg)
{
	printf("producer2\n");
	int i = 0;
	kn_msgque_writer_t writer = kn_open_writer(msgque);
	while(1){
		for(i = 0; i < 10000000; ++i){
			kn_msgque_write(writer,(void*)2,NULL);
			ATOMIC_INCREASE(&c2);
		}
		kn_msgque_flush(writer);
		while(c2 > 0){
			FENCE();
			kn_sleepms(0);
		}
	}
    return NULL;
}


void *consumer(void *arg)
{
	printf("consumer\n");
	kn_msgque_reader_t reader = kn_open_reader(msgque);
	void *msg;
	uint32_t tick = kn_systemms();
	while(1){
		kn_msgque_read(reader,&msg,-1);
		if(msg){ 
			++count;
			if((uint64_t)msg == 1)
				ATOMIC_DECREASE(&c1);
			else
				ATOMIC_DECREASE(&c2);	
			if(count == 5000000){
				uint32_t now = kn_systemms();
            				uint32_t elapse = (uint32_t)(now-tick);
				printf("pop %d/ms\n",count/elapse*1000);
				tick = now;
				count = 0;				
			}
		}
	}
    return NULL;
}

int main(){
	msgque = kn_new_msgque(64);
	kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_t t3 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_start_run(t1,producer1,NULL);
	kn_thread_start_run(t2,producer2,NULL);	
	kn_thread_start_run(t3,consumer,NULL);		
 	getchar();
	return 0;
}

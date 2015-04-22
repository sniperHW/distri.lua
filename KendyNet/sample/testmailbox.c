#include "kendynet.h"
#include "kn_time.h"
#include "kn_thread_mailbox.h"
#include "kn_thread.h"

volatile int count = 0;

kn_thread_mailbox_t mailbox2;

static void on_mail1(kn_thread_mailbox_t *from,void *mail){
		static uint32_t tick = 0;
		if(tick == 0){
			tick = kn_systemms();
		}
		++count;
		if(count == 1000000){
            			uint32_t now = kn_systemms();
            			uint32_t elapse = (uint32_t)(now-tick);
			printf("count:%d\n",(uint32_t)(count*1000/elapse));			
			count = 0;
			tick = kn_systemms();
		}
		kn_send_mail(*from,mail,NULL);
}

static void on_mail2(kn_thread_mailbox_t *from,void *mail){
		kn_send_mail(*from,mail,NULL);
}


void *routine1(void *arg)
{
	printf("routine1\n");
	engine_t p = kn_new_engine();
	kn_setup_mailbox(p,on_mail1);
	while(is_empty_ident(mailbox2)){
		FENCE();
		kn_sleepms(1);
	}
	int i = 0;
	for(; i < 10000; ++i){
		kn_send_mail(mailbox2,(void*)1,NULL);
	}
	kn_engine_run(p);
    return NULL;
}

void *routine2(void *arg)
{
	printf("routine2\n");
	engine_t p = kn_new_engine();
	mailbox2 = kn_setup_mailbox(p,on_mail2);
	kn_engine_run(p);
    return NULL;
}

//单向
void *routine11(void *arg)
{
	printf("routine11\n");
	while(is_empty_ident(mailbox2)){
		FENCE();
		kn_sleepms(1);
	}	
	int i;
	while(1){
		for(i = 0; i < 50000; ++i){
			kn_send_mail(mailbox2,(void*)1,NULL);
		}
		//kn_sleepms(100);
	}
    return NULL;
}

static void on_mail22(kn_thread_mailbox_t *from,void *mail){
		static uint32_t tick = 0;
		if(tick == 0){
			tick = kn_systemms();
		}
		++count;
		if(count == 1000000){
            			uint32_t now = kn_systemms();
            			uint32_t elapse = (uint32_t)(now-tick);
			printf("count:%d\n",(uint32_t)(count*1000/elapse));			
			count = 0;
			tick = kn_systemms();
		}
}

void *routine22(void *arg)
{
	printf("routine22\n");
	engine_t p = kn_new_engine();
	mailbox2 = kn_setup_mailbox(p,on_mail22);
	kn_engine_run(p);
    return NULL;
}


int main(){

	kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_start_wait(t1,routine11,NULL);
	kn_thread_start_wait(t2,routine22,NULL);		

	while(1){
		kn_sleepms(1000);		
	}	
	return 0;
}

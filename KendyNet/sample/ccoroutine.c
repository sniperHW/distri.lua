#include "uthread/uthread.h"


int utcount = 0;

void uthread_fun(void *arg){
	int i = 0;
	while(i++ < 10){
		printf("%lld,%d\n",ut_getcurrent().identity,(int)arg);
		ut_sleep(1000);
	}
	--utcount;
}

int main(){
	uscheduler_init(4096);
	int i = 0;
	for(; i < 10; ++i){
		utcount++;
		ut_spawn(uthread_fun,(void*)i);
	}	
	while(utcount > 0)
		uschedule();
	printf("finish\n");
	return 0;
}
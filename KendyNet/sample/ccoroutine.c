#include "uthread/uthread.h"


int utcount = 0;

void uthread_fun(void *arg){
	int i = 0;
	ut_dic_set(ut_getcurrent(),"key",(void*)rand(),NULL);
	while(i++ < 10){
		printf("%lld,%d,%d\n",ut_getcurrent().identity,(int)arg,(int)ut_dic_get(ut_getcurrent(),"key"));
		ut_sleep(1000);
	}
	--utcount;
}

int main(){
	uscheduler_init(8192);
	int i = 0;
	for(; i < 10; ++i){
		utcount++;
		ut_spawn(uthread_fun,(void*)i);
	}
	while(utcount > 0)
		uschedule();
	int activecount = uschedule();
	printf("finish,%d\n",activecount);
	return 0;
}
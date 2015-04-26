#include "kendynet.h"
#include "kn_timer.h"


struct userst{
	uint64_t expecttime;
	uint64_t ms;
};
engine_t proactor=NULL;

int timer_callback(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){
		struct userst *st = (struct userst*)ud;
		printf("%ld,%ld\n",kn_systemms64()-st->expecttime,st->ms);
		st->expecttime = kn_systemms64() + st->ms;
	}
	return 0;
}


int main(){


	
	proactor = kn_new_engine();
	
	struct userst st1,st2,st3,st4;
	st1.ms = 100;
	st1.expecttime = kn_systemms64()+st1.ms;
	
	st2.ms = 1005;
	st2.expecttime = kn_systemms64()+st2.ms;
	
	st3.ms = 10*60*1000;
	st3.expecttime = kn_systemms64()+st3.ms;
	
	st4.ms = 3600*1000;//+59*1000;
	st4.expecttime = kn_systemms64()+st4.ms;
	
	kn_reg_timer(proactor,st1.ms,timer_callback,&st1);
	kn_reg_timer(proactor,st2.ms,timer_callback,&st2);
	kn_reg_timer(proactor,st3.ms,timer_callback,&st3);
	kn_reg_timer(proactor,st4.ms,timer_callback,&st4);	
	
	while(1){
		kn_engine_run(proactor);	
	}
	return 0;
}

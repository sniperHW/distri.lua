#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "kendynet.h"
#include "kn_thread.h"
#include "kn_time.h"
#include "spinlock.h"

int value = 0;
spinlock_t lock;

void *routine1(void *arg)
{
	int i = 0;
	for(; i < 5000000; ++i){
		spin_lock(lock);
		++value;
		spin_unlock(lock);
	}
    return NULL;
}

void *routine2(void *arg)
{
	int i = 0;
	for(; i < 5000000; ++i){
		spin_lock(lock);
		--value;
		spin_unlock(lock);
	}
    return NULL;
}

int main(){	
	lock = spin_create();
	kn_thread_t t1 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t2 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t3 = kn_create_thread(THREAD_JOINABLE);
	kn_thread_t t4 = kn_create_thread(THREAD_JOINABLE);	
	kn_thread_start_run(t1,routine1,NULL);
	kn_thread_start_run(t2,routine2,NULL);
	kn_thread_start_run(t3,routine1,NULL);
	kn_thread_start_run(t4,routine2,NULL);	

	kn_thread_join(t1);
	kn_thread_join(t2);
	kn_thread_join(t3);
	kn_thread_join(t4);	
	printf("%d\n",value);
	return 0;
}



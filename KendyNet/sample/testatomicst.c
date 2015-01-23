#include "kn_thread.h"
#include "kn_time.h"
#include "kn_atomic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lockfree/atomic_st.h"

struct point
{
	volatile int x;
	volatile int y;
	volatile int z;
};

DECLARE_ATOMIC_TYPE(atomic_point,struct point);
	
struct atomic_point *g_points[1000];

void *SetRotine(void *arg)
{
    int idx = 0;
    int pos = 0;
	while(1)
	{
		struct point p;
		++pos;
		p.x = p.y = p.z = pos+1;
		atomic_point_set(g_points[idx],&p);
		//idx = (idx + 1)%100;
	}
}

void *GetRoutine(void *arg)
{
    int idx = 0;
	while(1)
	{
		struct point ret;
		atomic_point_get(g_points[idx],&ret);
		if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
		{
			printf("%d,%d,%d\n",ret.x,ret.y,ret.z);
			assert(0);
		}			
		//idx = (idx + 1)%100;
	}
}

int main()
{		
	struct point p;
	p.x = p.y = p.z = 1;
	int i;
	for(i = 0; i < 1000; ++i)
	{
		g_points[i] = atomic_point_new();
		atomic_point_set(g_points[i],&p);
	}
	RUN_THREAD(THREAD_JOINABLE,SetRotine,NULL);
	RUN_THREAD(THREAD_JOINABLE,GetRoutine,(void*)1);	
    	RUN_THREAD(THREAD_JOINABLE,GetRoutine,(void*)2);
	uint32_t tick = kn_systemms();
	while(1)
	{
		uint32_t new_tick = kn_systemms();
		if(new_tick - tick >= 1000)
		{
			printf("get:%d,set:%d,miss:%d\n",get_count,set_count,miss_count);
			get_count = set_count = miss_count = 0;
			tick = new_tick;
		}
		kn_sleepms(50);
	}
}
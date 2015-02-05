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
	int x;
	int y;
	int z;
};

DECLARE_ATOMIC_TYPE(atomic_point,struct point);
	
struct atomic_point *g_points[3];

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
		idx = (idx + 1)%3;
	}
}

void *GetRoutine(void *arg)
{
    	int idx = 0;
    	uint32_t tick = kn_systemms();
    	int c = 0;
	volatile struct point ret = {0};      	  	    
	while(1)
	{
		atomic_point_get(g_points[idx],&ret);
		if(ret.x != ret.y || ret.x != ret.z || ret.y != ret.z)
		{
			printf("%d,%d,%d\n",ret.x,ret.y,ret.z);
			assert(0);
		}
		if(++c >= 10000000){
			uint32_t now = kn_systemms();
            			uint32_t elapse = (uint32_t)(now-tick);
			printf("%d get %d/ms\n",(int)((uint64_t)arg),c/elapse*1000);
			tick = now;			
			c = 0;
		}						
		idx = (idx + 1)%3;
	}
}

int main()
{		
	struct point p;
	p.x = p.y = p.z = 1;
	int i;
	for(i = 0; i < 3; ++i)
	{
		g_points[i] = atomic_point_new();
		atomic_point_set(g_points[i],&p);
	}
	RUN_THREAD(THREAD_JOINABLE,SetRotine,NULL);
	RUN_THREAD(THREAD_JOINABLE,GetRoutine,(void*)1);	
    	RUN_THREAD(THREAD_JOINABLE,GetRoutine,(void*)2);
    	getchar();
    	return 0;
}	
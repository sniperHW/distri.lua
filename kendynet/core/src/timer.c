#include "timer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct wheel
{
	time_t   round_time;      //一轮执行完后的时间
	uint32_t cur_idx;
	uint32_t size;
    struct dlist wheel[0];
};

struct timer
{
	struct wheel *wheels[SIZE];
	time_t init_time;           //创建时的时间
};

//根据类型和消逝时间计算各级时间轮当前应该所处的idx
static inline uint32_t cal_index(time_t elapse,int8_t type)
{
	if(type == MIN) return elapse%60;
	else if(type == HOUR) return (elapse/60)%60;
	else if(type == DAY) return (elapse/3600)%24;
	else if(type == YEAR) return (elapse/(3600*24))%365;
	assert(0);
	return 0;
}

static uint32_t _mod[SIZE] = {60,60,24,365};
static uint32_t _round_time[SIZE] = {60,60*60,60*60*24,60*60*24*365};

void   init_timer_item(struct timer_item *item)
{
    item->dlnode.pre = item->dlnode.next = NULL;
    item->ud_ptr = NULL;
}

static inline void active(struct timer *timer,struct dlist *wheel)
{
    struct dnode *node = NULL;
    while((node = dlist_pop(wheel)) != NULL){
		struct timer_item *item = (struct timer_item*)node;
		item->callback(timer,item,item->ud_ptr);
	}
}

void update_timer(struct timer *timer,time_t now)
{
	time_t elapse = now - timer->init_time;//从定时器创建到现在消逝的时间
	int8_t type = MIN;
	for(; type < SIZE; ++type){
	    struct wheel *wheel = timer->wheels[type];
		uint32_t new_idx = cal_index(elapse,type);
		while(wheel->cur_idx != new_idx)
		{
			if(type == MIN)
				active(timer,&wheel->wheel[wheel->cur_idx]);
			else{
				//将wheel[i]中的内容整体移到type-1的cur_idx中
				struct wheel *lower = timer->wheels[type-1];
                dlist_move(&lower->wheel[0],&wheel->wheel[wheel->cur_idx]);
			}
			wheel->cur_idx = (wheel->cur_idx + 1) % _mod[type];
			if(wheel->cur_idx == 0){
			    //转了一圈
                wheel->round_time += _round_time[type];
                break;
			}
		}
		if(wheel->cur_idx != 0) return;
	}
}

static inline int32_t Add(struct wheel *wheel,struct timer_item *item,
						  time_t timeout,time_t elapse,int8_t type)
{
	if(timeout < wheel->round_time){
		uint32_t idx = cal_index(elapse,type);
		assert(idx < wheel->size);
        dlist_push(&wheel->wheel[idx],(struct dnode*)item);
		return 1;
	}
	return 0;
}

int8_t register_timer(struct timer *timer,struct timer_item *item,time_t timeout)
{
	time_t now = time(NULL);
	timeout = now + timeout;
	time_t elapse = now - timer->init_time;
	int8_t i = MIN;
	for( ; i < SIZE; ++i){
		if(Add(timer->wheels[i],item,timeout,elapse,i))
			return 0;
	}
	return -1;
}

void unregister_timer(struct timer_item *item)
{
    dlist_remove((struct dnode *)item);
}

struct wheel* new_wheel(uint32_t size)
{
    struct wheel *wheel = calloc(1,sizeof(*wheel) + size*sizeof(struct dlist));
	wheel->size = size;
	wheel->cur_idx = 0;
	int32_t i = 0;
    for(; i < size; ++i) dlist_init(&wheel->wheel[i]);
	return wheel;
}

struct timer* new_timer()
{
	struct timer *timer = calloc(1,sizeof(*timer));
	timer->init_time = time(NULL);
	int type = MIN;
	for( ; type < SIZE; ++type){
        timer->wheels[type] = new_wheel(_mod[type]);
        timer->wheels[type]->round_time = timer->init_time + _round_time[type];
	}
	return timer;
}

void   delete_timer(struct timer **timer)
{
	int32_t i = 0;
	for(; i < SIZE; ++i) free((*timer)->wheels[i]);
	free(*timer);
	*timer = NULL;
}

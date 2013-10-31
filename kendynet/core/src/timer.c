#include "timer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct wheel
{
	time_t   round_time;      //一轮执行完后的时间
	uint32_t cur_idx;
	uint32_t size;
	struct wheel *upper;      //上一级的wheel,当cur_idx达到size,将上一级中cur_idx中的内容移动到本wheel[0]中
	struct double_link wheel[0];
};

struct timer
{
	struct wheel *wheels[SIZE];
	time_t init_time;           //创建时的时间
};

//根据类型和消逝时间计算各级时间轮当前应该所处的idx
static inline uint32_t cal_index(time_t elapse,int8_t type)
{
	uint32_t idx;
	if(type == MIN){
		idx = elapse%60;
	}else if(type == HOUR){
		idx = (elapse/60)%60;
	}else if(type == DAY){
		idx = (elapse/3600)%24;
	}else if(type == YEAR){
		idx = (elapse/(3600*24))%365;
	}
	return idx;
}

static inline uint32_t get_mod(int8_t type){
	if(type == MIN)return 60;
	else if(type == HOUR)return 60;
	else if(type == DAY) return 24;
	else return 365;
}


static inline void active(struct timer *timer,struct double_link *wheel)
{
	struct double_link_node *node = NULL;
	while(node = double_link_pop(wheel))
	{
		struct timer_item *item = (struct timer_item*)node;
		item->callback(timer,item,item->ud_ptr);
	}
}

static inline void update_roundtime(struct wheel *wheel,int8_t type)
{
	if(type == MIN)  wheel->round_time += 60;
	if(type == HOUR) wheel->round_time += 60*60;
	if(type == DAY)  wheel->round_time += 60*60*24;
	if(type == YEAR) wheel->round_time += 60*60*24*365;
}


void update_timer(struct timer *timer,time_t now)
{
	time_t elapse = now - timer->init_time;//从定时器创建到现在消逝的时间
	struct wheel *wheel = timer->wheels[MIN];
	int8_t type = MIN;
	while(wheel){
		uint32_t new_idx = cal_index(elapse,type);
		uint32_t i = wheel->cur_idx;
		int8_t   beyond = 0;
		while(i != new_idx)
		{
			if(type == MIN){
				//触发回调函数
				active(timer,&wheel->wheel[i]);
			}else{
				//将wheel[i]中的内容整体移到type-1的cur_idx中
				struct wheel *lower = timer->wheels[type-1];
				double_link_move(&lower->wheel[0],&wheel->wheel[i]);
			}
			i = (i+1) % get_mod(type);
			if(i == 0){
				wheel->cur_idx = i;;//转了一圈
				break;
			}
		}
		if(wheel->cur_idx = 0){
			update_roundtime(wheel,type);
			wheel = wheel->upper;
			type += 1;
		}else
			wheel = NULL;
	}
}

static inline int32_t Add(struct wheel *wheel,
						  struct timer_item *item,
						  time_t timeout,
						  time_t elapse,
						  int8_t type
						  )
{
	if(timeout < wheel->round_time)
	{
		uint32_t idx = cal_index(elapse,type);
		assert(idx < wheel->size);
		double_link_push(&wheel->wheel[idx],(struct double_link_node*)item);
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
	double_link_remove((struct double_link_node *)item);
}

struct wheel* new_wheel(uint32_t size)
{
	struct wheel *wheel = calloc(1,sizeof(*wheel) + size*sizeof(struct double_link));
	wheel->size = size;
	wheel->cur_idx = 0;
	int32_t i = 0;
	for(; i < size; ++i)
		double_link_clear(&wheel->wheel[i]);
	return wheel;
}

struct timer* new_timer()
{
	struct timer *timer = calloc(1,sizeof(*timer));
	timer->init_time = time(NULL);

	timer->wheels[YEAR] = new_wheel(365);
	timer->wheels[YEAR]->round_time = timer->init_time + 60*60*24*365;
	timer->wheels[YEAR]->upper = NULL;

	timer->wheels[DAY] = new_wheel(24);
	timer->wheels[DAY]->round_time = timer->init_time + 60*60*24;
	timer->wheels[DAY]->upper = timer->wheels[YEAR];


	timer->wheels[HOUR] = new_wheel(60);
	timer->wheels[HOUR]->round_time = timer->init_time + 60*60;
	timer->wheels[HOUR]->upper = timer->wheels[DAY];

	timer->wheels[MIN] = new_wheel(60);
	timer->wheels[MIN]->round_time = timer->init_time + 60;
	timer->wheels[MIN]->upper = timer->wheels[HOUR];
	return timer;
}

void   delete_timer(struct timer **timer)
{
	int32_t i = 0;
	for(; i < SIZE; ++i)
		free((*timer)->wheels[i]);
	free(*timer);
	*timer = NULL;
}

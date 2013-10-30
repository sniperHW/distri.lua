#include <stdio.h>
#include <stdlib.h>
#include "timing_wheel.h"
#include "SysTime.h"

struct TimingWheel
{
	uint32_t precision;
	uint32_t slot_size;
	uint32_t last_update;
	uint32_t current;
	WheelItem_t   slot[0];
};

TimingWheel_t CreateTimingWheel(uint32_t precision,uint32_t max)
{
	uint32_t slot_size = max/precision;
	TimingWheel_t t = malloc(sizeof(*t) + (slot_size*sizeof(WheelItem_t)));
	if(!t)
		return NULL;
	uint32_t i = 0;
	for(; i < slot_size; ++i)
		t->slot[i] = NULL;
	t->slot_size = slot_size;
	t->precision = precision;
	t->current = 0;
	t->last_update = GetSystemMs();
	return t;
}

void DestroyTimingWheel(TimingWheel_t *t)
{
	//销毁所有剩余的定时器
	uint32_t i = 0;
	for( ; i < (*t)->slot_size; ++i)
	{
	   WheelItem_t cur = (*t)->slot[i];
	   while(cur)
       {
			WheelItem_t tmp = cur;
			cur = cur->next;
			if(tmp->on_destroy)
                tmp->on_destroy(tmp);
      }
	}
	free(*t);
	*t = NULL;
}

inline static void Add(TimingWheel_t t,uint32_t slot,WheelItem_t item)
{
	if(t->slot[slot])
	{
		t->slot[slot]->pre = item;
		item->next = t->slot[slot];
	}
	else
		item->next = item->pre = NULL;
	t->slot[slot] = item;
	item->slot = slot;
}

int  RegisterTimer(TimingWheel_t t,WheelItem_t item,uint32_t timeout)
{
	if(timeout > t->precision)
		timeout = t->precision;
	uint32_t now = GetSystemMs();
	int n = (now + timeout - t->last_update - t->precision)/t->precision;
	if(n >= t->slot_size)
		return -1;
	n = (t->current + n)%t->slot_size;
	Add(t,n,item);
	item->timing_wheel = t;
	return 0;
}

//激活slot中的所有事件
static void Active(TimingWheel_t t,uint32_t slot,uint32_t now)
{

	while(t->slot[slot])
	{
		WheelItem_t tmp = t->slot[slot];
		t->slot[slot] = tmp->next;
		if(tmp->next)
			tmp->next->pre = NULL;
		tmp->next = tmp->pre = NULL;
		tmp->timing_wheel = NULL;
		tmp->callback(t,tmp,now);
	}
}

int UpdateWheel(TimingWheel_t t,uint32_t now)
{
	uint32_t interval = now - t->last_update;
	if(interval < t->precision)
		return -1;
	interval = interval/t->precision;
	uint32_t i = 0;
	for( ; i < interval && i < t->slot_size; ++i)
		Active(t,(t->current+i)%t->slot_size,now);
	t->current = (t->current+i)%t->slot_size;
	t->last_update = now;//+= (interval*t->precision);


	return 0;
}

void UnRegisterTimer(WheelItem_t wit)
{
	if(!wit) return;
	TimingWheel_t t = wit->timing_wheel;
	if(!t) return;
	uint32_t slot = wit->slot;
	if(slot >= t->slot_size) return;

	if(t->slot[slot] == wit){
		//head
		t->slot[slot] = wit->next;
		if(wit->next) wit->next->pre = NULL;
	}else
	{
		if(wit->pre) wit->pre->next = wit->next;
		if(wit->next) wit->next->pre = wit->pre;
	}
	wit->pre = wit->next = NULL;
	wit->timing_wheel = NULL;
}

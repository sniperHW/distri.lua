#include "kendynet_private.h"
#include "kn_timer.h"
#include "kn_timer_private.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "kn_time.h"
#include "kn_dlist.h"

enum{
	wheel_sec = 0,  
	wheel_hour,     
	wheel_day,      
};

typedef struct {
	uint8_t  type;
	uint16_t cur;
	kn_dlist items[0]; 
}wheel;

#define wheel_size(T) (T==wheel_sec?1000:T==wheel_hour?3600:T==wheel_day?24:0)
#define precision(T) (T==wheel_sec?1:T==wheel_hour?1000:T==wheel_day?3600:0)

static wheel* wheel_new(uint8_t type){
	if(type >  wheel_day)
		return NULL;
	wheel *w = calloc(1,sizeof(*w)*wheel_size(type)*sizeof(kn_dlist));	
	w->type = type;
	w->cur = 0;
	uint16_t size = (uint16_t)wheel_size(type);
	uint16_t i = 0;
	for(; i < size; ++i){
		kn_dlist_init(&w->items[i]);
	}
	return w;	
}

typedef struct kn_timer{
	kn_dlist_node node;
	uint32_t      timeout;
	uint64_t      expire;
	int32_t       (*callback)(uint32_t,void*); 
	void         *ud;
}*kn_timer_t;

typedef struct wheelmgr{
	wheel 		*wheels[wheel_day+1];
	uint64_t     lasttime;
}*wheelmgr_t;


static inline void add2wheel(wheelmgr_t m,wheel *w,kn_timer_t t,uint64_t remain){
	uint64_t slots = wheel_size(w->type) - w->cur;
	if(w->type == wheel_day || slots > remain){
		uint16_t i = (w->cur + remain)%(wheel_size(w->type));
		kn_dlist_push(&w->items[i],(kn_dlist_node*)t);		
	}else{
		remain -= slots;
		remain /= wheel_size(w->type);
		return add2wheel(m,m->wheels[w->type+1],t,remain);		
	}
}

static inline void _reg(wheelmgr_t m,kn_timer_t t,uint64_t tick,wheel *w){
	assert(t->expire > tick);
	if(t->expire > tick)
		add2wheel(m,w?w:m->wheels[wheel_sec],t,t->expire - tick);
}

//将本级超时的定时器推到下级时间轮中
static inline void down(wheelmgr_t m,kn_timer_t t,uint64_t tick,wheel *w){
	assert(w->cur == 0);
	assert(t->expire >= tick);
	if(t->expire >= tick){
		uint64_t remain = (t->expire - tick) - wheel_size(w->type-1);
		remain /= precision(w->type);
		kn_dlist_push(&w->items[w->cur + remain],(kn_dlist_node*)t);		
	}	
}

//处理上一级时间轮
static inline void tickup(wheelmgr_t m,wheel *w,uint64_t tick){
	kn_timer_t t;
	kn_dlist *items = &w->items[w->cur];
	while((t = (kn_timer_t)kn_dlist_pop(items)))
		down(m,t,tick,m->wheels[w->type-1]);
	w->cur = (w->cur+1)%wheel_size(w->type);			
	if(w->cur == 0 && w->type != wheel_day){
		tickup(m,m->wheels[w->type+1],tick);
	}	
}

static void fire(wheelmgr_t m,uint64_t tick){
	kn_timer_t t;
	wheel *w = m->wheels[wheel_sec];			
	if((w->cur = (w->cur+1)%wheel_size(wheel_sec)) == 0)
		tickup(m,m->wheels[wheel_hour],tick);
	kn_dlist *items = &w->items[w->cur];		
	while((t = (kn_timer_t)kn_dlist_pop(items))){
		int32_t ret = t->callback(TEVENT_TIMEOUT,t->ud);
		if(ret >= 0 && ret <= MAX_TIMEOUT){
			if(ret > 0) t->timeout = ret;
			t->expire = tick + t->timeout;
			_reg(m,t,tick,NULL);
		}else{
			if(ret > 0){
				//todo: log
			}
			free(t);
		}
	}
}

void wheelmgr_tick(wheelmgr_t m,uint64_t now){
	while(m->lasttime != now)
		fire(m,++m->lasttime);
} 

kn_timer_t wheelmgr_register(wheelmgr_t m,uint32_t timeout,
					     int32_t(*callback)(uint32_t,void*),
					     void*ud){
	if(timeout == 0 || timeout > MAX_TIMEOUT || !callback)
		return NULL;
	uint64_t now = kn_systemms64();
	kn_timer_t t = calloc(1,sizeof(*t));
	t->timeout = timeout;
	t->expire = now + timeout;
	t->callback = callback;
	t->ud = ud;
	if(!m->lasttime) m->lasttime = now;
	_reg(m,t,now,NULL);
	return t;
}

wheelmgr_t wheelmgr_new(){
	wheelmgr_t t = calloc(1,sizeof(*t));
	int i = 0;
	for(; i < wheel_day+1; ++i)
		t->wheels[i] = wheel_new(i);
	return t;
}

void kn_del_timer(kn_timer_t t){
	kn_dlist_remove((kn_dlist_node*)t);
	t->callback(TEVENT_DESTROY,t->ud);
	free(t);
}

void wheelmgr_del(wheelmgr_t m){
	int i = 0;
	for(; i < wheel_day+1; ++i){
		uint16_t j = 0;
		uint16_t size = wheel_size(m->wheels[i]->type);
		for(; j < size; ++j){
			kn_dlist *items = &m->wheels[i]->items[j];
		    kn_timer_t t;
			while((t = (kn_timer_t)kn_dlist_pop(items))){
				t->callback(TEVENT_DESTROY,t->ud);
				free(t);				
			}
		}
		free(m->wheels[i]);
	}
	free(m);	
}

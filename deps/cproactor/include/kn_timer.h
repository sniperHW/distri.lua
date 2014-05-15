/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _KN_TIMER_H
#define _KN_TIMER_H
#include <time.h>
#include <stdint.h>
#include "minheap.h"


typedef struct kn_timer* kn_timer_t;
struct kn_timer_item;

//如果返回1则timer_callback调用完之后会释放掉timer_item
typedef int (*timer_callback)(kn_timer_t,struct kn_timer_item*,void*,uint64_t now);


kn_timer_t kn_new_timer();
void       kn_delete_timer(kn_timer_t);

//更新定时器
void kn_update_timer(kn_timer_t,uint64_t now);
struct kn_timer_item* kn_register_timer(kn_timer_t,struct kn_timer_item*,timer_callback,void *ud,uint64_t timeout);
void kn_unregister_timer(struct kn_timer_item **);    


/*    
#include "dlist.h"

//6级时间轮，最大到年，最小到豪秒
enum
{
    SEC = 0,
	MIN,
	HOUR,
	DAY,
	YEAR,
	SIZE,
};

struct timer;
struct timer_item
{
    struct dnode dlnode;
	void  *ud_ptr;
	void (*callback)(struct timer*,struct timer_item*,void*);
};

void   init_timer_item(struct timer_item*);
struct timer *new_timer();
void   delete_timer(struct timer**);
//更新定时器
void update_timer(struct timer*,time_t now);
int8_t register_timer(struct timer*,struct timer_item*,time_t timeout);
void unregister_timer(struct timer_item*);
*/

#endif

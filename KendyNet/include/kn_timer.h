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
#include "kendynet.h"

#define MAX_TIMEOUT (1000*3600*24-1)

enum{
    TEVENT_TIMEOUT = 1,
    TEVENT_DESTROY,
};

typedef struct kn_timer *kn_timer_t;

kn_timer_t kn_reg_timer(engine_t,
						uint64_t timeout,
						int32_t(*)(uint32_t,void*),
						void *ud);
						
void       kn_del_timer(kn_timer_t);//销毁timer并从timermgr中取消注册


//void*      kn_timer_getud(kn_timer_t);


#endif

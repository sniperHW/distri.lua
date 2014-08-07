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

typedef struct kn_timermgr *kn_timermgr_t;
typedef struct kn_timer *kn_timer_t;

typedef int  (*kn_cb_timer)(kn_timer_t);//如果返回1继续注册，否则不再注册

#endif

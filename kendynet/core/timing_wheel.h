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
#ifndef _TIMING_WHEEL_H
#define _TIMING_WHEEL_H
#include <stdint.h>
//简单时间轮

struct WheelItem;

typedef struct TimingWheel *TimingWheel_t;
typedef void   (*TimingWheel_callback)(TimingWheel_t,struct WheelItem*,uint32_t);
typedef void   (*WheelItem_OnDestroy)(struct WheelItem*);

typedef struct WheelItem
{
	struct WheelItem *pre;
	struct WheelItem *next;
	void *ud;
	TimingWheel_callback callback;
	uint32_t    slot;
	TimingWheel_t timing_wheel;
	WheelItem_OnDestroy on_destroy;
}*WheelItem_t;

/*
* precision:最小精度,以毫秒为单位
* max:最大能度量的时间,以毫秒为单位
*/
extern TimingWheel_t CreateTimingWheel(uint32_t precision,uint32_t max);
extern void DestroyTimingWheel(TimingWheel_t*);
extern int32_t  RegisterTimer(TimingWheel_t,WheelItem_t,uint32_t timeout);
extern void  UnRegisterTimer(/*TimingWheel_t,*/WheelItem_t);
extern int32_t  UpdateWheel(TimingWheel_t,uint32_t now);
#endif

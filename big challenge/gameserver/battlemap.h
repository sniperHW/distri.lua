#ifndef _BATTLEMAP_H
#define _BATTLEMAP_H
//战场地图

#include <stdint.h>
#include "game/aoi.h"
#include "refbase.h"
#include "asynnet/msgdisp.h"
#include "core/lua_util.h"
#include "core/timer.h"

typedef struct battlemap{
	struct map *aoimap;   //地图aoi管理结构
	luaObject_t _battle;  //战场规则在lua中管理 
}*battlemap_t;

//战场定时器回调
void battle_timer_callback(struct timer*,struct timer_item*,battlemap_t);

#endif
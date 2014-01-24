#ifndef _BATTLEREGISTER_H
#define _BATTLEREGISTER_H

#include "core/lua_util.h"
#include "core/timer.h"
#include "../avatar.h"
//战场报名管理器
typedef struct battleregister{
	struct timer_item _timer_item;
	luaObject_t       _registermgr;//在lua中管理报名处理
}*battleregister_t;


/*
* type:战场类型
*/
void battle_register(battleregister_t,player_t,uint16_t type);//战场报名

#endif
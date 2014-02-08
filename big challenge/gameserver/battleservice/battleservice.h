#ifndef _BATTLESERVICE_H
#define _BATTLESERVICE_H

#include "core/msgdisp.h"
#include "core/thread.h"
#include "llist.h"
#include "db/asyndb.h"
#include "../avatar.h"
#include "core/cstring.h"
#include "core/lua_util.h"

#define	MAX_BATTLE_SERVICE 64//每线程运行一个battle service

typedef struct battleservice
{
	volatile uint8_t   stop;
	thread_t           thd;
	msgdisp_t          msgdisp;
	atomic_32_t        player_count;    //此service上的玩家数量
	luaObject          battlemgr;       //实际的战场由lua对象管理
	player_cmd_handler player_handlers[MAX_CMD]; 
}*battleservice_t;

extern battleservice_t g_battleservices[MAX_BATTLE_SERVICE];

battleservice_t new_battleservice();
void destroy_battleservice(battleservice_t);

void build_player_cmd_handler(battleservice_t bservice);
void register_battle_cfunction(lua_State *L);


#endif
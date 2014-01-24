#include "battleregister.h"
#include "supperservice.h"

//定时器回调函数
static void battle_register_timer_callback(struct timer*,struct timer_item*,battleregister_t);

//请求创建战场
int luacreate_battle(lua_State *L)
{
	//由superservice选择一个battleservice将请求交给它
	return 0;
}
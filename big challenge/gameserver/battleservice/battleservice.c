#include "../common/cmd.h"
#include "battleservice.h"


static cmd_handler battle_cmd_handlers[MAX_CMD] = {0};

int32_t battle_processpacket(msgdisp_t disp,rpacket_t rpk);

static void *service_main(void *ud){
    battleservice_t service = (battleservice_t)ud;
    tls_create(MSGDISCP_TLS,(void*)service->msgdisp,NULL);
    tls_create(BATTLESERVICE_TLS,(void*)service,NULL);
    while(!service->stop){
        msg_loop(service->msgdisp,50);
    }
    return NULL;
}

void reg_battle_cmd_handler(uint16_t cmd,cmd_handler handler)
{
	if(cmd < MAX_CMD)
		battle_cmd_handlers[cmd] = handler;
}

battleservice_t new_battleservice()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L,"battlemgr.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
		exit(0);
	}
	register_battle_cfunction(L);
	battleservice_t bservice = calloc(1,sizeof(*bservice));
	if(0 != CALL_LUA_FUNC(L,"CreateBattleMgr",1))
	{
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
		exit(0);
	}
	bservice->battlemgr = create_luaObj(L,-1);
	bservice->msgdisp = new_msgdisp(NULL,NULL,NULL, NULL,battle_processpacket,NULL);
	bservice->thd = create_thread(THREAD_JOINABLE);
    thread_start_run(bservice->thd,service_main,(void*)bservice);
	return bservice;
}

void destroy_battleservice(battleservice_t bservice)
{
	//destroy_battleservice应该在gameserver关闭时调用，所以这里不需要做任何释放操作了
	bservice->stop = 1;
	thread_join(bservice->thd);
}
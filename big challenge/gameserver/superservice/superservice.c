#include "superservice.h"
#include "../common/cmd.h"
#include "../avatar.h"
#include "battleservice/battlemap.h"
#include "core/lua_util.h"

superservice_t g_superservice = NULL;
static super_cmd_handler cmd_handlers[MAX_CMD] = {0};

ident* players[MAX_PLAYER] = {0}; 

int32_t send2gate(wpacket_t wpk)
{
	if(!g_superservice) return -1;
	if(!wpk) return -1;
	if(is_vaild_ident(TO_IDENT(g_superservice->togate))) return -1;
	return asyn_send(g_superservice->togate,wpk);
}

void super_connect(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
    disp->bind(disp,0,sock,1,3*1000,0);//由系统选择poller
}

void super_connected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
	g_superservice->togate = sock;
}

void super_disconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port,uint32_t err)
{
	if(eq_ident(g_superservice->togate,sock))
		g_superservice->togate = make_empty_ident();
}


int32_t super_processpacket(msgdisp_t disp,msgsender sender,rpacket_t rpk)
{
	int32_t ret = 1;
	uint16_t cmd = rpk_peek_uint16(rpk);
	if(cmd >= CMD_CLIENT2GAME && cmd < CMD_CLIENT2GAME_END){
		player_t _player = cast2player(rpk_reverse_read_avatarid(rpk));
		if(_player){
			if(_player->_msgdisp != disp){
				if(0 == push_msg(_player->msgdisp,rpk))
					ret = 0;//不销毁rpk,由battleservice负责销毁
			}else{
				rpk_read_uint16(rpk);//丢弃cmd
				if(g_superservice->player_handlers[cmd])
					g_superservice->player_handlers[cmd](_player,rpk);
			}
			release_player(_player);	
		}
	}else if(cmd >= CMD_GATE2GAME && cmd < CMD_GATE2GAME_END){
		rpk_read_uint16(rpk);//丢弃cmd
		if(cmd_handlers[cmd])
			cmd_handlers[cmd](rpk);
	}else{
		//非法消息，记录日志
	}
    return ret;
}

void register_super_handle_function(uint16_t cmd,super_cmd_handler handler)
{
	if(cmd < MAX_CMD)
		cmd_handlers[cmd] = handler;
}

player_t cast2player(avatarid _avatarid)
{
	if(avatar_type(_avatarid) != type_player)
		return NULL;
	uint32_t index = avatar_index(_avatarid);
	if(index >= MAX_PLAYER) return NULL;
	ident *_ident = players[index];
	if(_ident){
		player_t _player = cast_2_refbase(*_ident);
		if(_player)
		{
			if(((avatar_t)_player)->_avatarid.data == _avatarid.data)
				return _player;
			else		
				release_player(_player);
		}
	}
	return NULL;
}

void start_superservice()
{
	g_superservice = calloc(1,sizeof(*g_superservice));

	//通过脚本读取战场类型配置
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L,"battledefine.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
		exit(0);
	}

	/*
	*  战场配置
	*  {
	*		{type1,maxplayer1,timeout1},	
	*		{type2,maxplayer2,timeout2},
	*		{type3,maxplayer3,timeout3},
	*  }
	*
	*/

}

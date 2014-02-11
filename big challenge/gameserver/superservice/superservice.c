#include "../common/cmd.h"
#include "../avatar.h"
#include "battleservice/battleservice.h"
#include "core/lua_util.h"
#include "superservice.h"

superservice_t g_superservice = NULL;
static cmd_handler super_cmd_handlers[MAX_CMD] = {0};

ident* players[MAX_PLAYER] = {0}; 

int32_t send2gate(wpacket_t wpk)
{
	if(!g_superservic || !wpk) return -1;
	if(!is_vaild_ident(TO_IDENT(g_superservice->togate))) return -1;
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


int32_t super_processpacket(msgdisp_t disp,rpacket_t rpk)
{
	uint16_t cmd = rpk_peek_uint16(rpk);
	if(cmd >= CMD_CLIENT2BATTLE && cmd <= CMD_CLIENT2BATTLE_END)
	{
		//将消息转发到battle
		avatarid _avatid = rpk_reverse_read_avatarid(rpk);
		battleservice_t battle = get_battle_by_index((uint8_t)_avatid.battleservice_id);
		if(battle && 0 == push_msg(battle->msgdisp,rpk))
			return 0;//不销毁rpk,由battleservice负责销毁
	}else{
		if(super_cmd_handlers[cmd]){
			rpk_read_uint16(rpk);//丢弃cmd
			super_cmd_handlers[cmd](rpk);
		}else{
			//记录日志
		}
	}
    return 1;
}

void reg_super_cmd_handler(uint16_t cmd,cmd_handler handler)
{
	if(cmd < MAX_CMD)
		super_cmd_handlers[cmd] = handler;
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
}

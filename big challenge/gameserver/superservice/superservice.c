#include "superservice.h"
#include "../common/cmd.h"
#include "../avatar.h"
#include "battleservice/battlemap.h"

superservice_t g_superservice = NULL;

static super_cmd_handler cmd_handlers[MAX_CMD] = {0}; 

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
	uint16_t cmd = rpk_peek_uint16(rpk);
	if(cmd >= CMD_CLIENT2GAME && cmd < CMD_CLIENT2GAME_END){
		uint32_t palyerid = reverse_read_uint32(rpk);
		player_t player = cast2player(palyerid);
		if(player){
			if(player->_msgdisp != disp){
				if(0 == push_msg(player->msgdisp,rpk))
					return 0;//不销毁rpk,由battleservice负责销毁
				return 1;
			}else{
				rpk_read_uint16(rpk);//丢弃cmd
				player_handlers[cmd](rpk);
			}	
		}
	}else if(cmd >= CMD_GATE2GAME && cmd < CMD_GATE2GAME_END){
		rpk_read_uint16(rpk);//丢弃cmd
		cmd_handlers[cmd](rpk);
	}else{
		//非法消息，记录日志
	}
    return 1;
}

void register_super_handle_function(uint16_t cmd,super_cmd_handler handler)
{
	if(cmd < MAX_CMD)
		cmd_handlers[cmd] = handler;
}

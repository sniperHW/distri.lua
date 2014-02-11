#include "agentservice.h"
#include "core/tls.h"
#include "togame/togame.h"
#include "common/tls_define.h"

static void *service_main(void *ud){
    agentservice_t service = (agentservice_t)ud;
    tls_create(MSGDISCP_TLS,(void*)service->msgdisp,NULL);
    tls_create(AGETNSERVICE_TLS,(void*)service,NULL);
    while(!service->stop){
        msg_loop(service->msgdisp,50);
    }
    return NULL;
}


agentplayer_t new_agentplayer(agentservice_t service,sock_ident sock)
{
	int32_t id = get_id(service->_idmgr);
	if(id == -1)
		return NULL;
	agentplayer_t player = service->players[id];
	player->session.aid = service->agentid;
	player->session.identity = service->identity;
	player->session.sessionid = (uint16_t)id;
	player->state = agent_init;
	player->con = sock;
	service->identity = (service->identity+1)&0X7FFF;
	return player;
}

void release_agentplayer(agentservice_t service,agentplayer_t player)
{
	release_id(service->_idmgr,(int32_t)player->sessionid.sessionid);
	player->session.data = 0;
	player->state = agent_unusing;
}

agentplayer_t get_agentplayer(agentservice_t service,agentsession session)
{
	agentplayer_t ply = service->players[session.sessionid];
	if(ply->session.data != session.data) return NULL;
	if(ply->status == agent_unusing) return NULL;
	return ply;
}


static void agent_connected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port)
{
	agentservice_t service = get_thd_agentservice();
	agentplayer_t ply = new_agentplayer(service);
	if(!ply)
	{
		//发送一个消息，通知系统繁忙然后关闭连接
		wpacket_t wpk = wpk_create(64,0);
		wpk_write_uint16(wpk,CMD_GAME_BUSY);
		asyn_send(sock,wpk);
		asynsock_close(sock);
	}else
	{
		asynsock_set_ud(sock,(void*)ply->session.data);
	}
}

statci void agent_disconnected(msgdisp_t disp,sock_ident sock,const char *ip,int32_t port,uint32_t err)
{
	agentservice_t service = get_thd_agentservice();
	agentsession session;
	session.data = (uint32_t)asynsock_get_ud(sock);
	agentplayer_t ply = get_agentplayer(service,session);
	if(ply){
		if(ply->status == agent_playing || ply->status == agent_creating)){
			//通知gameserver玩家连接断开

		}
		release_agentplayer(service,ply);
	}
}


int32_t agent_processpacket(msgdisp_t disp,rpacket_t rpk)
{
	uint16_t cmd = rpk_peek_uint16(rpk);
	if(cmd == CMD_PLY_CONNECTING)
	{
		//绑定到asynnet
		service->msgdisp->bind(service->msgdisp,0,sock,0,30*1000,0);//由系统选择poller
	}else
	{
		if(cmd >= CMD_CLIENT2GATE && cmd < CMD_CLIENT2GATE_END){
			rpk_read_uint16(rpk);//丢掉命令码


		}else if(cmd >= CMD_GAME2CLIENT && cmd < CMD_GAME2CLIENT_END){
			uint16_t size = reverse_read_uint16(rpk);//这个包需要发给多少个客户端
		    //在栈上创建一个rpacket_t用于读取需要广播的客户端
		    rpacket_t r = rpk_create_skip(rpk,size*sizeof(agentsession)+sizeof(size));
		    //将rpk中用于广播的信息丢掉
		    rpk_dropback(rpk,size*sizeof(agentsession)+sizeof(size));
		    int i = 0;
		    wpacket_t wpk = wpk_create_by_rpacket(rpk);
		    //发送给所有需要接收的客户端
		    for( ; i < size; ++i)
		    {
		        agentsession session;
		        session.data = rpk_read_uint32(r);
		        agentplayer_t ply = get_agentplayer(service,session);
		        if(ply)	
					asyn_send(ply->con,wpk);
		    }
		    rpk_destroy(&r);
		}else{
			agentservice_t service = get_thd_agentservice();
			sock_ident sock = CAST_2_SOCK(sender);
			agentsession session;
			session.data = (uint32_t)asynsock_get_ud(sock);
			agentplayer_t ply = get_agentplayer(service,session);
			if(ply && ply->status == agent_playing)
			{
				//转发到gameserver
				send2game(wpk_create_by_rpacket(rpk));
			}

		}
	}
    return 1;
}


agentservice_t new_agentservice(uint8_t agentid,asynnet_t asynet){
	agentservice_t service = calloc(1,sizeof(*service));
	service->agentid = agentid;
	service->_idmgr = new_idmgr(1,MAX_ANGETPLAYER);
	service->msgdisp = new_msgdisp(asynet,
                             	   NULL,
                                   agent_connected,
                                   agent_disconnected,
                                   agent_processpacket,
                                   NULL);
	service->thd = create_thread(THREAD_JOINABLE); 
	thread_start_run(service->thd,service_main,(void*)service);
	return service;
}


void destroy_agentservice(agentservice_t s)
{
	//destroy_agentservice应该在gateserver关闭时调用，所以这里不需要做任何释放操作了
	s->stop = 1;
	thread_join(s->thd);
}

agentservice_t get_thd_agentservice()
{
	return (agentservice_t)tls_get(AGETNSERVICE_TLS);
}
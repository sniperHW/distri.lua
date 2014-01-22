#ifndef _AGENTSERVICE_H
#define _AGENTSERVICE_H

#include "../../agentsession.h"
#include "core/msgdisp.h"
#include "core/thread.h"
#include "llist.h"

#define MAX_ANGETPLAYER   8192
#define AGETNSERVICE_TLS  1 

typedef struct agentservice
{
	volatile uint8_t stop;
	uint8_t     agentid;        //0-7
	thread_t    thd;            //运行本agentservice的线程
	struct      llist idpool;
	msgdisp_t   msgdisp;
	agentplayer players[8192];
	uint16_t    identity;
}*agentservice_t;

agentservice_t new_agentservice(uint8_t agentid,asynnet_t asynet);
void           destroy_agentservice(agentservice_t);

//获得当前线程的agentservice
agentservice_t get_thd_agentservice();


agentplayer_t get_agentplayer(agentservice_t service,agentsession session);


#endif
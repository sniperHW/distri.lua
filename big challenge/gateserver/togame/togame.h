#ifndef _TOGAME_H
#define _TOGAME_H

#include "../../agentsession.h"
#include "core/msgdisp.h"
#include "core/thread.h"

typedef struct toGame
{
	volatile uint8_t stop;
	thread_t    thd;            //运行本agentservice的线程
	struct      llist idpool;
	msgdisp_t   msgdisp;
	sock_ident  togame;
}*toGame_t;

extern toGame_t g_togame;
void send2game(wpacket_t wpk);

#endif
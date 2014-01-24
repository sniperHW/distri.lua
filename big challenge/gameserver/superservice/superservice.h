#ifndef _SUPERSERVICE_H
#define _SUPERSERVICE_H

#include "core/msgdisp.h"
#include "core/thread.h"
#include "llist.h"
#include "db/asyndb.h"
#include "../avatar.h"
#include "core/cstring.h"

typedef struct superservice
{
	volatile uint8_t stop;
	thread_t    thd;
	msgdisp_t   msgdisp;
	sock_ident  togate;   //到gate的套接口
	asyndb_t    asydb;
}*superservice_t;


extern superservice_t g_superservice;

superservice_t create_superservice();
void destroy_superservice(superservice_t*);

int32_t send2gate(wpacket_t);

typedef void (*super_cmd_handler)(rpacket_t);

void    register_super_handle_function(uint16_t cmd,super_cmd_handler);//注册消息处理函数

player_t create_player(string_t actname,uint32_t gateident);

player_t find_player_by_actname(string_t actname);

#endif
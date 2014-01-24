#ifndef _AVATAR_H
#define _AVATAR_H

#include "game/aoi.h"
#include "core/msgdisp.h"
#include "common/agentsession.h"
#include "core/refbase.h"
#include "core/cstring.h"

struct battlemap;

enum{
	avatar_player = 1,
	avatar_npc,
};

typedef struct avatar{
	aoi_object aoi;
	uint8_t    avatar_type;
	struct battlemap *_battlemap;
}avatar,*avatar_t;


typedef struct player{
	avatar       _avatar;	
	agentsession _agentsession;
	string_t     _actname;
	msgdisp_t    _msgdisp;
	void (*player_fn_destroy)(void*);
}player,*player_t;

player_t cast2player(uint32_t);

typedef void (*player_cmd_handler)(player_t,rpacket_t);


void register_palyer_handle_function(uint16_t cmd,player_cmd_handler);//注册消息处理函数

extern player_cmd_handler player_handlers[MAX_CMD]; 

#endif
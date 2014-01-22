#ifndef _AVATAR_H
#define _AVATAR_H

#include "game/aoi.h"
#include "core/msgdisp.h"
#include "common/agentsession.h"
#include "core/refbase.h"

//superservice中的player
typedef struct super_player{
	agentsession _agentsession;
	msgdisp_t    msgdisp;//当前为player处理消息的消息分离器
	void (*super_fn_destroy)(void*);
}super_player,*super_player_t;


enum{
	avatar_player = 1,
	avatar_npc,
};

typedef struct avatar{
	aoi_object aoi;
	uint8_t    avatar_type;
}avatar,*avatar_t;


typedef struct battle_player{
	struct refbase ref; 
	avatar _avatar;
	super_player_t superply;//对应的super_player
}battle_player,*battle_player_t;



#endif
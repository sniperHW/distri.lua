#ifndef _AVATAR_H
#define _AVATAR_H

#include "game/aoi.h"
#include "core/msgdisp.h"
#include "common/agentsession.h"
#include "core/refbase.h"
#include "core/cstring.h"
#include "core/rpacket.h"
#include "core/wpacket.h"
#include "core/dlist.h"

#define MAX_PLAYER    8191*8  //superservice最多容纳8191*8个玩家对象
#define MAX_NONPLAYER 1<<18   //battleservice最多容纳1<<18个非玩家对象，非玩家对象包括npc,掉落物等

typedef struct avatarid{
	union{
		struct{
			uint64_t index:18;
			uint64_t avattype:8;        
			uint64_t checksum:38;     
		};
		uint64_t     data;
	};
}avatarid;

enum{
	avatar_player = 1,
	avatar_npc,
};

typedef struct avatar{
	struct refbase ref;
	aoi_object aoi;
	avatarid   _avatarid;
}avatar,*avatar_t;

typedef struct player{
	avatar       _avatar;
	agentsession _agentsession;
	string_t     _actname;
	msgdisp_t    _msgdisp;
}player,*player_t;

static inline uint16_t avatar_type(avatarid _avatarid)
{
	return (uint16_t)_avatarid.avattype;
}

static inline uint32_t avatar_index(avatarid _avatarid)
{
	return (uint32_t)_avatarid.index;
}

static inline avatarid rpk_read_avatarid(rpacket_t rpk)
{
	avatarid _avatarid;
	_avatarid.data = rpk_read_uint64(rpk);
	return _avatarid;
}

static inline avatarid rpk_reverse_read_avatarid(rpacket_t rpk)
{
	avatarid _avatarid;
	_avatarid.data = reverse_read_uint64(rpk);
	return _avatarid;	
}

static inline void wpk_write_avatarid(wpacket_t wpk,avatarid _avatarid)
{
	wpk_write_uint64(wpk,_avatarid.data);
}

player_t cast2player(avatarid);
void     release_player(player_t _player);

typedef void (*player_cmd_handler)(player_t,rpacket_t);

#endif
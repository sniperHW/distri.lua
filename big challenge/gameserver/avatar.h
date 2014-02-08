#ifndef _AVATAR_H
#define _AVATAR_H

#include "core/msgdisp.h"
#include "common/agentsession.h"
#include "core/refbase.h"
#include "core/cstring.h"
#include "core/rpacket.h"
#include "core/wpacket.h"

#define MAX_PLAYER    8191*8  //superservice最多容纳8191*8个玩家对象

typedef struct avatarid{
	uint16_t super_player_index;
	union{
		struct{
			uint32_t isplayer:1;
			uint32_t battleservice_id:6;        
			uint32_t map_id:14;
			uint32_t ojb_index:11;     
		};
		uint32_t     data;
	};
}avatarid;


typedef struct player{
	agentsession _agentsession;
	string_t     _actname;
	msgdisp_t    _msgdisp;
	uint16_t     _index;
}player,*player_t;

static inline avatarid rpk_read_avatarid(rpacket_t rpk)
{
	avatarid _avatarid;
	_avatarid.super_player_index = rpk_read_uint16(rpk);
	_avatarid.data = rpk_read_uint32(rpk);
	return _avatarid;
}

static inline avatarid rpk_reverse_read_avatarid(rpacket_t rpk)
{
	avatarid _avatarid;
	reverse_read(rpk,&_avatarid,3);
	return _avatarid;	
}

static inline void wpk_write_avatarid(wpacket_t wpk,avatarid _avatarid)
{
	wpk_write_uint16(wpk,_avatarid.super_player_index);
	wpk_write_uint32(wpk,_avatarid.data);
}


/*enum{
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
*/
#endif
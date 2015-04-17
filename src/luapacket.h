#ifndef _LUAPACKET_H
#define _LUAPACKET_H

#include "lua/lua_util.h"
#include "buffer.h"
#include "kn_common_define.h"
#include "packet.h"
#include "rpacket.h"
#include "wpacket.h"
#include "rawpacket.h"

#define LUAPACKET_METATABLE "luapacket_metatable"

typedef struct{
	packet_t _packet;
}lua_packet,*lua_packet_t;

inline static lua_packet_t lua_getluapacket(lua_State *L, int index) {
    return (lua_packet_t)luaL_testudata(L, index, LUAPACKET_METATABLE);
}

void push_luapacket(lua_State *L,packet_t);

void reg_luapacket(lua_State *L);

#endif

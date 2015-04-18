#ifndef  _LUA_UTIL_PACKET_H
#define _LUA_UTIL_PACKET_H

#include "wpacket.h"
#include "rpacket.h"

struct lua_State;


const char *lua_pack_table(wpacket_t wpk,lua_State *L,int index);

int lua_unpack_table(rpacket_t rpk,lua_State *L);

#endif
#ifndef _LUA_BYTEBUFFER_H
#define _LUA_BYTEBUFFER_H

#include "lua_util.h"
#include "buffer.h"
#include "kn_common_define.h"

#define BYTEBUFFER_METATABLE "bytebuffer_metatable"

typedef struct{
	buffer_t raw_buffer;
}lua_bytebuffer,*lua_bytebuffer_t;

void reg_luabytebuffer(lua_State *L);

#endif

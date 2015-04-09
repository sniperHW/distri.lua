#ifndef _LUASOCKET_H
#define _LUASOCKET_H
#include "kendynet.h"
#include "connection.h"
#include "lua_util.h"

typedef struct {
	handle_t      sock;
	connection_t streamconn;
	int                type;
	luaRef_t      luaObj;
	int                listening;
}*luasocket_t;

void reg_luasocket(lua_State *L);

#endif

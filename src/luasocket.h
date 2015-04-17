#ifndef _LUASOCKET_H
#define _LUASOCKET_H
#include "kendynet.h"
#include "connection.h"
#include "datagram.h"
#include "lua/lua_util.h"

typedef struct {
	handle_t      sock;
	union{
		connection_t streamconn;
		datagram_t   datagram;
	};
	int                type;
	luaRef_t      luaObj;
	int                listening;
}*luasocket_t;

void reg_luasocket(lua_State *L);

#endif

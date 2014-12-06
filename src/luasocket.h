#ifndef _LUASOCKET_H
#define _LUASOCKET_H
#include "kendynet.h"
#include "stream_conn.h"
#include "lua_util.h"

typedef struct {
	handle_t      sock;
	stream_conn_t streamconn;
	int                type;
	luaRef_t      luaObj;
}*luasocket_t;

void reg_luasocket(lua_State *L);

#endif

#ifndef _PUSH_CALLBACK_H
#define _PUSH_CALLBACK_H
#include "lua/lua_util.h"

#ifndef push_callback
#define push_callback(L,fmt,...) LuaCall(L,"push_c_callback",fmt,__VA_ARGS__)
#endif


#ifndef push_obj_callback
#define push_obj_callback(L,fmt,...) LuaCall(L,"push_c_obj_callback",fmt,__VA_ARGS__)
#endif


#endif

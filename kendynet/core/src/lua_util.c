#include "lua_util.h"

void luaObjcet_destroy(void *ptr)
{
	luaObject_t o = (luaObject_t)ptr;
	luaL_unref(o->L,LUA_REGISTRYINDEX,o->rindex);	
	free(ptr);
}

luaObject_t create_luaObj(lua_State *L,int idx)
{
	luaObject_t o = calloc(1,sizeof(*o));
	ref_init((struct refbase*)o,luaObjcet_destroy,1);
	o->L = L;
	int top = lua_gettop(L);
	if(top != idx)
	{
		lua_pushvalue(L,idx);
		o->rindex = luaL_ref(L,LUA_REGISTRYINDEX);
	}else
		o->rindex = luaL_ref(L,LUA_REGISTRYINDEX);
	return o;
}

void release_luaObj(luaObject_t o)
{
	ref_decrease((struct refbase*)o);
}

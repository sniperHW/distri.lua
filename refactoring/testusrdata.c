#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>

#define UD_METATABLE "ud_metatable"

typedef struct{
	int val;
}*ud_t;

static ud_t _getud(lua_State *L, int index) {
    ud_t ud = (ud_t)luaL_checkudata(L, index, UD_METATABLE);
    return ud;
}

static int _newud(lua_State *L) {
    ud_t ud = (ud_t) lua_newuserdata(L, sizeof(*ud));
    luaL_getmetatable(L, UD_METATABLE);
    lua_setmetatable(L, -2);
	ud->val = 100;
	return 1;
}

static int _gc(lua_State *L){
	printf("_gc\n");
	return 0;
}

static int _hello(lua_State *L){
	ud_t ud = _getud(L,1);
	printf("hello:%d\n",ud->val);
	int rindex = luaL_ref(L,LUA_REGISTRYINDEX);
	lua_remove(L,2);
	lua_rawgeti(L,LUA_REGISTRYINDEX,rindex);
	luaL_unref(L,LUA_REGISTRYINDEX,rindex);
	return 1;
}

int main(){
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
    luaL_Reg ud_mt[] = {
        {"__gc", _gc},
        {NULL, NULL}
    };
    
    luaL_Reg ud_methods[] = {
        {"hello", _hello},
        {NULL, NULL}
    };
    luaL_newmetatable(L, UD_METATABLE);
    luaL_setfuncs(L, ud_mt, 0);
    luaL_newlib(L, ud_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    
	lua_newtable(L);	
	lua_pushstring(L,"ud");
	lua_pushcfunction(L,&_newud);
	lua_settable(L, -3);				
	lua_setglobal(L,"ud");    	
	
	if (luaL_dofile(L,"testusrdata.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}	
	return 0;
}

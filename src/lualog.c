#include "lualog.h"

extern engine_t g_engine;


#define LUALOG_METATABLE "lualog_metatable"

typedef struct{
	logfile_t _f;
}lua_log,*lua_log_t;

inline static lua_log_t lua_getlualog(lua_State *L, int index) {
    return (lua_log_t)luaL_testudata(L, index, LUALOG_METATABLE);
}

int lua_create_logfile(lua_State *L){
	const char *filename = lua_tostring(L,1);
	logfile_t f = create_logfile(filename);
	lua_log_t l = (lua_log_t)lua_newuserdata(L, sizeof(*l));
	l->_f = f;
	luaL_getmetatable(L, LUALOG_METATABLE);
	lua_setmetatable(L, -2);
	return 1;
}

int lua_write_log(lua_State *L){
	lua_log_t l = lua_getlualog(L,1);
	logfile_t f = l->_f;
	int loglev = lua_tointeger(L,2);
	const char *str = lua_tostring(L,3);
	LOG(f,loglev,"%s",str);
	printf("%s\n",str);
	return 0;
}

int lua_sys_log(lua_State *L){
	int loglev = lua_tointeger(L,1);
	const char *str = lua_tostring(L,2);
	SYS_LOG(loglev,"%s",str);
	printf("%s\n",str);
	return 0;	
}

#define REGISTER_CONST(___N) do{\
		lua_pushstring(L, #___N);\
		lua_pushinteger(L, ___N);\
		lua_settable(L, -3);\
}while(0)

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

void reg_lualog(lua_State *L){
	lua_newtable(L);	
	REGISTER_CONST(LOG_INFO);
	REGISTER_CONST(LOG_ERROR);
	
    luaL_Reg log_methods[] = {
        {"Log", lua_write_log},		
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUALOG_METATABLE);
    luaL_newlib(L, log_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_Reg l[] = {
        {"New",lua_create_logfile},
        {"SysLog",lua_sys_log},                        
        {NULL, NULL}
    };
    luaL_newlib(L,l);	
	lua_setglobal(L,"CLog");		
}

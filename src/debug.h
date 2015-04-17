#include "lua/lua_util.h"

//需要调用luaL_loadfilex的地方用my_luaL_loadfilex替代
int my_luaL_loadfilex(lua_State *L, const char *filename,const char *mode);

int debug_init();


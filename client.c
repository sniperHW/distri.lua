#include "lua.h"  
#include "lauxlib.h"  
#include "lualib.h"  
extern void BindFunction(lua_State *lState); 


int main()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if(luaL_dofile(L,"client.lua"))
	{
		const char *err = lua_tostring(L,-1);
		printf("%s\n",err);
		return 0;
	}
	BindFunction(L);
	lua_getglobal(L,"mainloop");
	if(lua_pcall(L,0,0,0) != 0)
	{
		const char *err = lua_tostring(L,-1);
		printf("%s\n",err);
		lua_pop(L,1);
	}
	
	return 0;
}

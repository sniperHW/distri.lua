#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>


/*i:符号整数
* u:无符号整数
* s:字符串
* b:布尔值
* n:lua number
* r:lua ref
* p:指针(lightuserdata)
*/    

const char *lua_call(lua_State *L,const char *func,const char *fmt,...){
	va_list vl;
	int narg,nres;
	
}



/*
int main(){
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L,"testsche.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}	
	return 0;
}*/

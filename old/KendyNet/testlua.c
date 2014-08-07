#include "lua/lua_util.h"


int testfun(lua_State *L){
	//printf("gettop:%d\n",lua_gettop(L));
	printf("testfun\n");
	int array[3];
	GET_ARRAY(L,1,array,lua_tointeger);
	printf("%d\n",array[0]);
	printf("%d\n",array[1]);
	printf("%d\n",array[2]);
	printf("%d\n",(int)lua_tointeger(L,2));		
	return 0;
}

int main(){
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	lua_register(L,"testfun",testfun);
	if (luaL_dofile(L,"test.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}

	if(CALL_LUA_FUNC(L,"newQue",1)){
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}	
	luaObject_t obj = create_luaObj(L,-1);	

	CALL_OBJ_FUNC(obj,"show",1);//,lua_pushstring(obj->L,"hello"));

	printf("%d\n",(int)lua_tonumber(L,-1));

	printf("%d\n",lua_gettop(L));
	return 0;
}

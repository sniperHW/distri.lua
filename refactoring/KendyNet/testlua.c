#include "lua_util.h"

int main(){
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L,"test.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	printf("--------test call lua function--------------\n");
	const char *err;
	if((err = LuaCall0(L,"fun0",0))){
		printf("lua error0:%s\n",err);
	}
	if((err = LuaCall1(L,"fun1",0,
					   lua_pushnumber(L,1)
	))){
		printf("lua error1:%s\n",err);
	}
	if((err = LuaCall2(L,"fun2",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2)
	))){
		printf("lua error2:%s\n",err);
	}
	if((err = LuaCall3(L,"fun3",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3)
	))){
		printf("lua error3:%s\n",err);
	}	
	if((err = LuaCall4(L,"fun4",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3),
					   lua_pushnumber(L,4)					   
	
	))){
		printf("lua error4:%s\n",err);
	}	
	if((err = LuaCall5(L,"fun5",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3),
					   lua_pushnumber(L,4),
					   lua_pushnumber(L,5) 	
	
	))){
		printf("lua error5:%s\n",err);
	}
	if((err = LuaCall6(L,"fun6",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3),
					   lua_pushnumber(L,4),
					   lua_pushnumber(L,5), 
					   lua_pushnumber(L,6)		
	
	))){
		printf("lua error6:%s\n",err);
	}
	if((err = LuaCall0(L,"none",0))){
		printf("call none:%s\n",err);
	}
	
	printf("--------------test call lua table function----------------------\n");	
	lua_getglobal(L,"ttab");
	luaTabRef_t tabRef = create_luaTabRef(L,-1);
	
	if((err = CallLuaTabFunc0(tabRef,"fun0",0))){
		printf("lua error0:%s\n",err);
	}
	if((err = CallLuaTabFunc1(tabRef,"fun1",0,
					   lua_pushnumber(L,1)
	))){
		printf("lua error1:%s\n",err);
	}
	if((err = CallLuaTabFunc2(tabRef,"fun2",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2)
	))){
		printf("lua error2:%s\n",err);
	}
	if((err = CallLuaTabFunc3(tabRef,"fun3",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3)
	))){
		printf("lua error3:%s\n",err);
	}	
	if((err = CallLuaTabFunc4(tabRef,"fun4",0,
					   lua_pushnumber(L,1),
					   lua_pushnumber(L,2),
					   lua_pushnumber(L,3),
					   lua_pushnumber(L,4)					   
	
	))){
		printf("lua error4:%s\n",err);
	}	
	if((err = CallLuaTabFunc0(tabRef,"none",0))){
		printf("call none:%s\n",err);
	}	
	
	release_luaTabRef(tabRef);
	
	printf("------------test enum lua table-----------\n");	
	lua_getglobal(L,"ttab2");
	tabRef = create_luaTabRef(L,-1);
	LuaTabEnum(tabRef){
		lua_State *L = luaTabRef_LState(tabRef);
		printf("%s",lua_tostring(L,EnumKey));
		luaTabRef_t subtabRef = create_luaTabRef(L,EnumVal);
		LuaTabEnum(subtabRef){
			printf(",");
			printf("%d",(int)lua_tonumber(L,EnumVal));
		}
		printf("\n");
	}
													
	return 0;
}

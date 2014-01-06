#ifndef _LUA_UTIL_H
#define _LUA_UTIL_H
#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h> 
#include <stdio.h>
#include <stdlib.h>

//call lua function
#define CALL_LUA_FUNC(LUASTATE,FUNCNAME,RET)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			int __result;\
			do __result = lua_pcall(LUASTATE,0,RET,0);\
			while(0);\
		__result;})

#define CALL_LUA_FUNC1(LUASTATE,FUNCNAME,RET,ARG1)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			int __result;\
			do __result = lua_pcall(LUASTATE,1,RET,0);\
			while(0);\
		__result;})

#define CALL_LUA_FUNC2(LUASTATE,FUNCNAME,RET,ARG1,ARG2)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			ARG2;\
			int __result;\
			do __result = lua_pcall(LUASTATE,2,RET,0);\
			while(0);\
		__result;})
		
#define CALL_LUA_FUNC3(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			ARG2;\
			ARG3;\
			int __result;\
			do __result = lua_pcall(LUASTATE,3,RET,0);\
			while(0);\
		__result;})
		
#define CALL_LUA_FUNC5(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			ARG2;\
			ARG3;\
			ARG4;\
			int __result;\
			do __result = lua_pcall(LUASTATE,4,RET,0);\
			while(0);\
		__result;})
		
#define CALL_LUA_FUNC6(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4,ARG5)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			ARG2;\
			ARG3;\
			ARG4;\
			ARG5;\
			int __result;\
			do __result = lua_pcall(LUASTATE,4,RET,0);\
			while(0);\
		__result;})

#define CALL_LUA_FUNC4(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4,ARG5,ARG6)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			ARG2;\
			ARG3;\
			ARG4;\
			ARG5;\
			ARG6;\
			int __result;\
			do __result = lua_pcall(LUASTATE,4,RET,0);\
			while(0);\
		__result;})

		
//argument pass c to lua		
#define PUSH_STRING(LUASTATE,STR) lua_pushstring(LUASTATE,STR)
#define PUSH_NUMBER(LUASTATE,NUM) lua_pushnumber(LUASTATE,NUM)		
#define PUSH_LUSRDATA(LUASTATE,LUD) lua_pushlightuserdata(LUASTATE,(void*)LUD)
#define PUSH_BOOL(LUASTATE,FLAG) lua_pushboolean(LUASTATE,FLAG)
#define PUSH_NIL(LUASTATE) lua_pushnil(LUASTATE)

#define PUSH_TABLE1(LUASTATE,VAL1)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(L,-2,1);\
			}while(0);
			
#define PUSH_TABLE2(LUASTATE,VAL1,VAL2)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(L,-2,1);\
				VAL2;\
				lua_rawseti(L,-2,2);\
			}while(0);
			
#define PUSH_TABLE3(LUASTATE,VAL1,VAL2,VAL3)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(L,-2,1);\
				VAL2;\
				lua_rawseti(L,-2,2);\
				VAL3;\
				lua_rawseti(L,-2,3);\
			}while(0);

#define PUSH_TABLE4(LUASTATE,VAL1,VAL2,VAL3,VAL4)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(L,-2,1);\
				VAL2;\
				lua_rawseti(L,-2,2);\
				VAL3;\
				lua_rawseti(L,-2,3);\
				VAL4;\
				lua_rawseti(L,-2,4);\
			}while(0);
			
#define GET_ARRAY(LUASTATE,IDX,ARRAY,POP)\
			do{\
				int len = luaL_len(LUASTATE,IDX);\
				int i = 1;\
				for(; i <= len; ++i)\
				{\
					lua_rawgeti(L,IDX-i+1,i);\
					ARRAY[i-1] = POP(LUASTATE,-1);\
				}\
			}while(0);	


			
#include "core/refbase.h"

typedef struct luaObject
{
	struct refbase base;
	lua_State      *L;
	int 		   rindex;
}*luaObject_t;

luaObject_t create_luaObj(lua_State *L);
void        release_luaObj(luaObject_t);


#define CALL_OBJ_FUNC(OBJ,FUNCNAME,RET)\
		({\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FUNCNAME);\
			lua_gettable(OBJ->L,-2);\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			int __result;\
			do __result = lua_pcall(OBJ->L,1,RET,0);\
			while(0);\
			lua_pop(OBJ->L,1);\
		__result;})
		
#define CALL_OBJ_FUNC1(OBJ,FUNCNAME,RET,ARG1)\
		({\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FUNCNAME);\
			lua_gettable(OBJ->L,-2);\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			int __result;\
			ARG1;\
			do __result = lua_pcall(OBJ->L,2,RET,0);\
			while(0);\
			lua_pop(OBJ->L,1);\
		__result;})

#define CALL_OBJ_FUNC2(OBJ,FUNCNAME,RET,ARG1,ARG2)\
		({\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FUNCNAME);\
			lua_gettable(OBJ->L,-2);\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			int __result;\
			ARG1;\
			ARG2;\
			do __result = lua_pcall(OBJ->L,3,RET,0);\
			while(0);\
			lua_pop(OBJ->L,1);\
		__result;})
		
#define CALL_OBJ_FUNC3(OBJ,FUNCNAME,RET,ARG1,ARG2,ARG3)\
		({\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FUNCNAME);\
			lua_gettable(OBJ->L,-2);\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			int __result;\
			ARG1;\
			ARG2;\
			ARG3;\
			do __result = lua_pcall(OBJ->L,4,RET,0);\
			while(0);\
			lua_pop(OBJ->L,1);\
		__result;})

#define CALL_OBJ_FUNC4(OBJ,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4)\
		({\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FUNCNAME);\
			lua_gettable(OBJ->L,-2);\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			int __result;\
			ARG1;\
			ARG2;\
			ARG3;\
			ARG4;\
			do __result = lua_pcall(OBJ->L,5,RET,0);\
			while(0);\
			lua_pop(OBJ->L,1);\
		__result;})			

#define GET_OBJ_FIELD(OBJ,FIELD,TYPE,POP)\
		({\
			TYPE __result;\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FIELD);\
			lua_gettable(OBJ->L,-2);\
			do __result = (TYPE)POP(OBJ->L,-1);\
			while(0);\
			lua_pop(OBJ->L,2);\
		__result;})
		
#define SET_OBJ_FIELD(OBJ,FIELD,PUSH,VAL)\
		do{\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FIELD);\
			PUSH(OBJ->L,VAL);\
			lua_settable(OBJ->L,-3);\
			lua_pop(OBJ->L,1);\
		}while(0);

#define PUSH_LUAOBJECT(LUASTATE,OBJ) lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex)	


#endif
/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _LUA_UTIL_H
#define _LUA_UTIL_H
#ifdef USE_LUAJIT
#include <luajit-2.0/lua.h>  
#include <luajit-2.0/lauxlib.h>  
#include <luajit-2.0/lualib.h>
#else
#include <lua.h>  
#include <lauxlib.h>  
#include <lualib.h>
#endif 
#include <stdio.h>
#include <stdlib.h>

//call lua function
#define LuaCall0(LUASTATE,FUNCNAME,RET)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,0,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})

#define LuaCall1(LUASTATE,FUNCNAME,RET,ARG1)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,1,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})

#define LuaCall2(LUASTATE,FUNCNAME,RET,ARG1,ARG2)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;ARG2;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,2,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})
		
#define LuaCall3(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;ARG2;ARG3;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,3,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})
		
#define LuaCall4(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;ARG2;ARG3;ARG4;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,4,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})
		
#define LuaCall5(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4,ARG5)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;ARG2;ARG3;ARG4;ARG5;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,5,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})
		
#define LuaCall6(LUASTATE,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4,ARG5,ARG6)\
		({\
			lua_getglobal(LUASTATE,FUNCNAME);\
			ARG1;ARG2;ARG3;ARG4;ARG5;ARG6;\
			const char * ___err = NULL;\
			if(lua_pcall(LUASTATE,6,RET,0)){\
				___err = lua_tostring(LUASTATE,1);\
				lua_pop(LUASTATE,1);\
			}___err;})


//lua表的一个引用
typedef struct
{
	lua_State      *L;
	int 		   rindex;	
}luaTabRef_t;

static inline luaTabRef_t create_luaTabRef(lua_State *L,int idx)
{
	luaTabRef_t o;
	lua_pushvalue(L,idx);
	o.L = L;
	o.rindex = luaL_ref(L,LUA_REGISTRYINDEX);
	return o;
}

static inline int  isVaild_TabRef(luaTabRef_t o){
	return o.rindex != LUA_REFNIL;
}

static inline void release_luaTabRef(luaTabRef_t *o)
{
	if(o->rindex != LUA_REFNIL){
		luaL_unref(o->L,LUA_REGISTRYINDEX,o->rindex);
		o->L = NULL;
		o->rindex = LUA_REFNIL;
	}
}

static inline lua_State *luaTabRef_LState(luaTabRef_t o){
	return o.L;
}

#define CallLuaTabFunc0(TABREF,FUNCNAME,RET)\
		({\
			lua_State *___L = (TABREF).L;\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			lua_pushstring(___L,FUNCNAME);\
			lua_gettable(___L,-2);\
			lua_remove(___L,-2);\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			const char *___err = NULL;\
			if(lua_pcall(___L,1,RET,0)){\
				___err = lua_tostring(___L,1);\
				lua_pop(___L,1);\
			}___err;})
			
#define CallLuaTabFunc1(TABREF,FUNCNAME,RET,ARG1)\
		({\
			lua_State *___L = (TABREF).L;\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			lua_pushstring(___L,FUNCNAME);\
			lua_gettable(___L,-2);\
			lua_remove(___L,-2);\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			const char *___err = NULL;\
			ARG1;\
			if(lua_pcall(___L,2,RET,0)){\
				___err = lua_tostring(___L,1);\
				lua_pop(___L,1);\
			}___err;})
			
#define CallLuaTabFunc2(TABREF,FUNCNAME,RET,ARG1,ARG2)\
		({\
			lua_State *___L = (TABREF).L;\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			lua_pushstring(___L,FUNCNAME);\
			lua_gettable(___L,-2);\
			lua_remove(___L,-2);\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			const char *___err = NULL;\
			ARG1;ARG2;\
			if(lua_pcall(___L,3,RET,0)){\
				___err = lua_tostring(___L,1);\
				lua_pop(___L,1);\
			}___err;})	
			
#define CallLuaTabFunc3(TABREF,FUNCNAME,RET,ARG1,ARG2,ARG3)\
		({\
			lua_State *___L = (TABREF).L;\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			lua_pushstring(___L,FUNCNAME);\
			lua_gettable(___L,-2);\
			lua_remove(___L,-2);\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			const char *___err = NULL;\
			ARG1;ARG2;ARG3;\
			if(lua_pcall(___L,4,RET,0)){\
				___err = lua_tostring(___L,1);\
				lua_pop(___L,1);\
			}___err;})
			
#define CallLuaTabFunc4(TABREF,FUNCNAME,RET,ARG1,ARG2,ARG3,ARG4)\
		({\
			lua_State *___L = (TABREF).L;\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			lua_pushstring(___L,FUNCNAME);\
			lua_gettable(___L,-2);\
			lua_remove(___L,-2);\
			lua_rawgeti(___L,LUA_REGISTRYINDEX,(TABREF).rindex);\
			const char *___err = NULL;\
			ARG1;ARG2;ARG3;ARG4;\
			if(lua_pcall(___L,5,RET,0)){\
				___err = lua_tostring(___L,1);\
				lua_pop(___L,1);\
			}___err;})

#define EnumKey -2
#define EnumVal -1				
#define LuaTabEnum(TABREF)\
			for(lua_rawgeti(TABREF.L,LUA_REGISTRYINDEX,TABREF.rindex),lua_pushnil(TABREF.L);\
				({\
					int __result;\
					do __result = lua_next(TABREF.L,-2);\
					while(0);\
					if(!__result)lua_pop(TABREF.L,1);\
					__result;});lua_pop(TABREF.L,1))


#define GET_OBJ_FIELD(TABREF,FIELD,TYPE,POP)\
		({\
			TYPE __result;\
			lua_rawgeti(TABREF.L,LUA_REGISTRYINDEX,TABREF.rindex);\
			lua_pushstring(TABREF.L,FIELD);\
			lua_gettable(TABREF.L,-2);\
			lua_remove(TABREF.L,-2);\
			do __result = (TYPE)POP(TABREF.L,-1);\
			while(0);\
		__result;})
		
#define PushLuaTabRef(LUASTATE,TABREF)\
        do{\
            lua_rawgeti((TABREF).L,LUA_REGISTRYINDEX,(TABREF).rindex);\
            if(LUASTATE != (TABREF).L){\
                lua_xmove((TABREF).L,LUASTATE,1);\
            }\
        }while(0)		
														
/*
#define PUSH_TABLE1(LUASTATE,VAL1)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(LUASTATE,-2,1);\
			}while(0);
			
#define PUSH_TABLE2(LUASTATE,VAL1,VAL2)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(LUASTATE,-2,1);\
				VAL2;\
				lua_rawseti(LUASTATE,-2,2);\
			}while(0);
			
#define PUSH_TABLE3(LUASTATE,VAL1,VAL2,VAL3)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(LUASTATE,-2,1);\
				VAL2;\
				lua_rawseti(LUASTATE,-2,2);\
				VAL3;\
				lua_rawseti(LUASTATE,-2,3);\
			}while(0);

#define PUSH_TABLE4(LUASTATE,VAL1,VAL2,VAL3,VAL4)\
			do{\
				lua_newtable(LUASTATE);\
				VAL1;\
				lua_rawseti(LUASTATE,-2,1);\
				VAL2;\
				lua_rawseti(LUASTATE,-2,2);\
				VAL3;\
				lua_rawseti(LUASTATE,-2,3);\
				VAL4;\
				lua_rawseti(LUASTATE,-2,4);\
			}while(0);

#ifdef USE_LUAJIT

#define GET_ARRAY(LUASTATE,IDX,ARRAY,POP)\
			do{\
				int len = lua_objlen(LUASTATE,IDX);\
				int i = 1;\
				for(; i <= len; ++i)\
				{\
					lua_rawgeti(LUASTATE,1,i);\
					ARRAY[i-1] = POP(LUASTATE,-1);\
					lua_pop(L,1);\
				}\
			}while(0);

#else
			
#define GET_ARRAY(LUASTATE,IDX,ARRAY,POP)\
			do{\
				int len = lua_rawlen(LUASTATE,IDX);\
				int i = 1;\
				for(; i <= len; ++i)\
				{\
					lua_rawgeti(LUASTATE,1,i);\
					ARRAY[i-1] = POP(LUASTATE,-1);\
					lua_pop(L,1);\
				}\
			}while(0);	

#endif
#define GET_OBJ_FIELD(OBJ,FIELD,TYPE,POP)\
		({\
			TYPE __result;\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FIELD);\
			lua_gettable(OBJ->L,-2);\
			lua_remove(OBJ->L,-2);\
			do __result = (TYPE)POP(OBJ->L,-1);\
			while(0);\
		__result;})
		
#define SET_OBJ_FIELD(OBJ,FIELD,PUSH,VAL)\
		do{\
			lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
			lua_pushstring(OBJ->L,FIELD);\
			PUSH(OBJ->L,VAL);\
			lua_settable(OBJ->L,-3);\
			lua_pop(OBJ->L,1);\
		}while(0);

#define PUSH_LUAOBJECT(LUASTATE,OBJ)\
        do{\
            lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex);\
            if(LUASTATE != OBJ->L){\
                lua_xmove(OBJ->L,LUASTATE,1);\
            }\
        }while(0)

#define GETGLOBAL_NUMBER(LUASTATE,NAME)\
		({\
			int __result;\
			lua_getglobal(LUASTATE,NAME);\
			do __result = lua_tonumber(LUASTATE,-1);\
			while(0);\
		__result;})
		
#define GETGLOBAL_STRING(LUASTATE,NAME)\
		({\
			const char *__result;\
			lua_getglobal(LUASTATE,NAME);\
			do __result = lua_tostring(LUASTATE,-1);\
			while(0);\
		__result;})
		
#define GETGLOBAL_OBJECT(LUASTATE,NAME)\
		({\
			luaObject_t __result;\
			lua_getglobal(LUASTATE,NAME);\
			do __result = create_luaObj(LUASTATE,-1);\
			while(0);\
		__result;})

#define LUAOBJECT_ENUM(OBJ)\
			for(lua_rawgeti(OBJ->L,LUA_REGISTRYINDEX,OBJ->rindex),lua_pushnil(OBJ->L);\
				({\
					int __result;\
					do __result = lua_next(OBJ->L,-2);\
					while(0);\
					if(!__result)lua_pop(OBJ->L,1);\
					__result;});lua_pop(OBJ->L,1))
*/
#endif

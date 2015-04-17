#include "lua/lua_util.h"
#include "minheap.h"
#include "log.h"
#include "kn_time.h"

#define LUAMINHEAP_METATABLE "luaminheap_metatable"

struct stluaheap_ele{
	struct heapele base;
	uint64_t            n;
	//uint64_t            insert_time;
	luaRef_t            luaObj;
};

typedef struct{
	minheap_t m;
}*lua_minheap_t;

static int8_t less(struct heapele*l,struct heapele*r){//if l < r return 1,else return 0	
	return ((struct stluaheap_ele*)l)->n < ((struct stluaheap_ele*)r)->n ? 1:0;
}

static int luaminheap_new(lua_State *L){
	lua_minheap_t m = (lua_minheap_t)lua_newuserdata(L, sizeof(*m));
	m->m = minheap_create(65535,less);
	luaL_getmetatable(L, LUAMINHEAP_METATABLE);
	lua_setmetatable(L, -2);
	return 1;
}

inline static lua_minheap_t lua_getluaminheap(lua_State *L, int index) {
    return (lua_minheap_t)luaL_testudata(L, index, LUAMINHEAP_METATABLE);
}

static void _clear_fun(struct heapele *ele){
	release_luaRef(&((struct stluaheap_ele*)ele)->luaObj);
	free(ele);
}

static int destroy_minheap(lua_State *L) {
	lua_minheap_t m = lua_getluaminheap(L,1);
	minheap_clear(m->m,_clear_fun);
	minheap_destroy(&m->m);
   	return 0;
}

static int _insert(lua_State *L){
	lua_minheap_t m = lua_getluaminheap(L,1);
	if(!m){
		return luaL_error(L,"arg1 should be minheap");
	}
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");	
	luaRef_t luaObj = toluaRef(L,2);
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	uint64_t n = lua_tointeger(L,3);	
	do{
		struct stluaheap_ele *heap_ele = NULL;
		const char *error = LuaRef_Get(luaObj,"sp","heapele",&heap_ele);
		if(error){
			SYS_LOG(LOG_ERROR,"error on lua minheap _insert:%s\n",error);
			break;		
		}
		if(heap_ele)
			break;
		heap_ele = calloc(1,sizeof(*heap_ele));
		error = LuaRef_Set(luaObj,"sp","heapele",heap_ele);
		if(error){
			free(heap_ele);
			SYS_LOG(LOG_ERROR,"error on lua minheap _insert:%s\n",error);
			break;	
		}
		heap_ele->luaObj = luaObj;
		heap_ele->n = n;
		//heap_ele->insert_time = kn_systemms64();
		minheap_insert(m->m,(struct heapele*)heap_ele);
		lua_pushboolean(L,1);
		return 1;
	}while(0);
	release_luaRef(&luaObj);
	lua_pushboolean(L,0);
	return 1;	

}

static int _remove(lua_State *L){
	lua_minheap_t m = lua_getluaminheap(L,1);
	if(!m){
		return luaL_error(L,"arg1 should be minheap");
	}
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");	
	luaRef_t luaObj = toluaRef(L,2);
	struct stluaheap_ele *heap_ele = NULL;	
	do{
		const char *error = LuaRef_Get(luaObj,"sp","heapele",&heap_ele);
		if(error){
			SYS_LOG(LOG_ERROR,"error on lua minheap _remove:%s\n",error);
			break;		
		}
		if(!heap_ele)
			break;
		error = LuaRef_Set(luaObj,"sp","heapele",NULL);
		if(error){
			SYS_LOG(LOG_ERROR,"error on lua minheap _remove:%s\n",error);
			break;	
		}
		minheap_remove(m->m,(struct heapele*)heap_ele);
	}while(0);
	if(heap_ele){
		release_luaRef(&heap_ele->luaObj);
		free(heap_ele);
	}
	release_luaRef(&luaObj);
	return 0;
}

static int _pop(lua_State *L){
	lua_minheap_t m = lua_getluaminheap(L,1);
	if(!m){
		return luaL_error(L,"arg1 should be minheap");
	}
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");	
	uint64_t n = lua_tointeger(L,2);
	int i = 1;
	struct stluaheap_ele* min = (struct stluaheap_ele*)minheap_min(m->m);
	if(min && min->n <= n){
		lua_newtable(L);
		while(min && min->n <= n){
			LuaRef_Set(min->luaObj,"sp","heapele",NULL);
			minheap_popmin(m->m);
			push_LuaRef(L,min->luaObj);
			release_luaRef(&min->luaObj);
			free(min);
			lua_rawseti(L,-2,i++);
			min = (struct stluaheap_ele*)minheap_min(m->m);
		}
		return 1;
	}
	return 0;
}

static int _change(lua_State *L){
	lua_minheap_t m = lua_getluaminheap(L,1);
	if(!m){
		return luaL_error(L,"arg1 should be minheap");
	}
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");	
	luaRef_t luaObj = toluaRef(L,2);
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");
	uint64_t n = lua_tointeger(L,3);	
	do{
		struct stluaheap_ele *heap_ele = NULL;
		const char *error = LuaRef_Get(luaObj,"sp","heapele",&heap_ele);
		if(error){
			SYS_LOG(LOG_ERROR,"error on lua minheap _insert:%s\n",error);
			break;		
		}
		if(heap_ele)
			break;
		heap_ele->n = n;
		minheap_change(m->m,(struct heapele*)heap_ele);
	}while(0);
	release_luaRef(&luaObj);		
	return 0;
}

void reg_luaminheap(lua_State *L){
    luaL_Reg minheap_mt[] = {
        {"__gc", destroy_minheap},
        {NULL, NULL}
    };

    luaL_Reg minheap_methods[] = {
        {"Insert", _insert},
        {"Remove", _remove},
        {"Pop",_pop},
        {"Change",_change},                  
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMINHEAP_METATABLE);
    luaL_setfuncs(L, minheap_mt, 0);

    luaL_newlib(L, minheap_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_Reg l[] = {
        {"New",luaminheap_new},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
    lua_setglobal(L,"CMinHeap");    	
}
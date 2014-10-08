#include "luaredis.h"

extern __thread engine_t g_engine;

static const char *errmsg[] = {
	NULL,
	"connect error",
};


static void cb_connect(redisconn_t conn,int err,void *ud){
	luaRef_t   *cbObj = (luaRef_t*)ud;
	const char *e = errmsg[err == 0 ? 0:1];
	const char *error = LuaCallTabFuncS(*cbObj,"__cb_connect","ps",conn,e);
	if(error) printf("error on lua redis __cb_connect:%s\n",error);
	if(error || e){	
		release_luaRef(cbObj);
		free(cbObj);
	}			
}

static void cb_disconnected(redisconn_t conn,void *ud){
	printf("cb_disconnected\n");	
	luaRef_t   *cbObj = (luaRef_t*)ud;
	const char *error = LuaCallTabFuncS(*cbObj,"__on_disconnected",NULL);
	if(error){
		printf("error on lua redis __on_disconnected:%s\n",error);		
	}
	release_luaRef(cbObj);
	free(cbObj);	
}

int lua_redis_connect(lua_State *L){
	const char *ip = lua_tostring(L,1);
	uint16_t    port = (uint16_t)lua_tointeger(L,2);
	luaRef_t   *cbObj = calloc(1,sizeof(*cbObj)); 	
	*cbObj = toluaRef(L,3);	
	int ret = kn_redisAsynConnect(g_engine,ip,port,cb_connect,cb_disconnected,(void*)cbObj);
	if(ret != 0){
		release_luaRef(cbObj);
		free(cbObj);
		lua_pushstring(L,"connect to redis error");
	}else
		lua_pushnil(L);
	return 1; 
}

static void build_resultset(struct redisReply* reply,lua_State *L){
	if(reply->type == REDIS_REPLY_INTEGER){
		lua_pushinteger(L,(int32_t)reply->integer);
	}else if(reply->type == REDIS_REPLY_STRING){
		lua_pushstring(L,reply->str);
	}else if(reply->type == REDIS_REPLY_ARRAY){
		lua_newtable(L);
		int i = 0;
		for(; i < reply->elements; ++i){
			build_resultset(reply->element[i],L);
			lua_rawseti(L,-2,i+1);
		}
	}else{
		lua_pushnil(L);
	}
}

void redis_command_cb(redisconn_t conn,struct redisReply* reply,void *pridata)
{
	luaRef_t *cbObj = (luaRef_t*)pridata;
	const char * error;
	if(!reply || reply->type == REDIS_REPLY_NIL){
		if((error = LuaCallTabFunc(*cbObj,"__callback","sp","reply nil",NULL))){
			printf("redis_command_cb:%s\n",error);			
		}		
	}else if(reply->type == REDIS_REPLY_ERROR){
		if((error = LuaCallTabFunc(*cbObj,"__callback","sp",reply->str,NULL))){
			printf("redis_command_cb:%s\n",error);			
		}		
	}else{		
		int __result;
		lua_State *__L = cbObj->L;
		int __oldtop = lua_gettop(__L);
		lua_rawgeti(__L,LUA_REGISTRYINDEX,cbObj->rindex);
		lua_pushstring(__L,"__callback");
		lua_gettable(__L,-2);
		lua_insert(__L,-2);
		lua_pushnil(__L);
		build_resultset(reply,__L);
		__result = lua_pcall(__L,3,0,0);
		if(__result){
			error = lua_tostring(__L,-1);
			if(error){
				printf("error on redis_command_cb:%s\n",error);
			}	
		}
		lua_settop(__L,__oldtop);
	} 	
	release_luaRef(cbObj);
	free(cbObj);
}

int lua_redisCommand(lua_State *L){
	redisconn_t conn = lua_touserdata(L,1);
	const char *cmd = lua_tostring(L,2);		
	do{
		if(!cmd || strcmp(cmd,"") == 0){
			lua_pushboolean(L,0);
			break;
		}		
		luaRef_t   *cbObj = calloc(1,sizeof(*cbObj)); 	
		*cbObj = toluaRef(L,3);			
		if(REDIS_OK!= kn_redisCommand(conn,cmd,redis_command_cb,cbObj)){
			release_luaRef(cbObj);
			free(cbObj);			
			lua_pushboolean(L,0);
		}else
			lua_pushboolean(L,1);
	}while(0);	
	return 1;
}


int lua_redisClose(lua_State *L){
	redisconn_t conn = lua_touserdata(L,1);
	kn_redisDisconnect(conn);
	return 0;
}

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

void reg_luaredis(lua_State *L){
	lua_newtable(L);		
	REGISTER_FUNCTION("close",lua_redisClose);
	REGISTER_FUNCTION("redis_connect",lua_redis_connect);	
	REGISTER_FUNCTION("redisCommand",lua_redisCommand);		
	lua_setglobal(L,"CRedis");		
}



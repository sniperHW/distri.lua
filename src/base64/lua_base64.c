#include <stdio.h>
#include <string.h>
#include "lua/lua_util.h"
#include "base64.h"


#define default_encodebuf (65535*2)
#define  default_decodebuf (65535)
static __thread char encodebuf[default_encodebuf];
static __thread unsigned char decodebuf[default_decodebuf];

static int lua_encode(lua_State *L){
	size_t len;
	int      outlen = 0;
	char   *output;
	unsigned char *input = (unsigned char*)lua_tolstring(L,-1,&len);
	if(len*2 > default_encodebuf){
		output = calloc(1,len*2);
	}else{
		output = encodebuf;
	}
	base64_encode(input, (int)len, output,&outlen);
	lua_pushlstring(L,output,(size_t)outlen);
	if(output != encodebuf) free(output);	
	return 1;	
}

static int lua_decode(lua_State *L){
	size_t len;
	char *input = (char*)lua_tolstring(L,-1,&len);
	int outlen = 0;
	unsigned char   *output;
	if(len > default_decodebuf){
		output = calloc(1,len*2);
	}else{
		output = decodebuf;
	}
	base64_decode(input, (int)len, output,&outlen);
	lua_pushlstring(L,(char*)output,(size_t)outlen);
	if(output != decodebuf) free(output);
	return 1;	
}

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

 void reg_luabase64(lua_State *L){
	lua_newtable(L);		
	REGISTER_FUNCTION("encode",lua_encode);
	REGISTER_FUNCTION("decode",lua_decode);	
	lua_setglobal(L,"CBase64");
}   

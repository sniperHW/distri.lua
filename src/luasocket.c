#include "kendynet.h"
#include "stream_conn.h"
#include "luasocket.h"
#include "luapacket.h"
//#include "luabytebuffer.h"

extern __thread engine_t g_engine;
static __thread lua_State *g_L;
enum{
	_SOCKET = 1,
	_STREAM_CONN = 2,
};

typedef struct {
	handle_t      sock;
	stream_conn_t streamconn;
	int           type;
	luaRef_t      luaObj;	
}*luasocket_t;

static int luasocket_new1(lua_State *L){
	luaRef_t obj = toluaRef(L,1);
	int domain = lua_tointeger(L,2);
	int type = lua_tointeger(L,3);
	int protocal = lua_tointeger(L,4);
	handle_t sock = kn_new_sock(domain,type,protocal);	
	if(!sock){
		lua_pushnil(L);
		return 1;
	}
	luasocket_t luasock = calloc(1,sizeof(*luasock));
	luasock->type = _SOCKET;
	luasock->sock = sock;
	luasock->luaObj = obj;
    kn_sock_setud(sock,luasock);	
	lua_pushlightuserdata(L,luasock);	
	return 1;
}

static int luasocket_new2(lua_State *L){
	luaRef_t obj = toluaRef(L,1);	
	handle_t sock =   lua_touserdata(L,2);
	luasocket_t luasock = calloc(1,sizeof(*luasock));
	luasock->type = _SOCKET;
	luasock->sock = sock;
	luasock->luaObj = obj;	
    kn_sock_setud(sock,luasock);	
	lua_pushlightuserdata(L,luasock);	
	return 1;	
}


static int  on_packet(stream_conn_t c,packet_t p){
	luasocket_t luasock = (luasocket_t)stream_conn_getud(c);
	luaRef_t  *obj = &luasock->luaObj;	
	int __result;
	lua_State *__L = obj->L;
	int __oldtop = lua_gettop(__L);
	lua_rawgeti(__L,LUA_REGISTRYINDEX,obj->rindex);
	lua_pushstring(__L,"__on_packet");
	lua_gettable(__L,-2);
	lua_insert(__L,-2);
	push_luapacket(__L,p);
	__result = lua_pcall(__L,2,0,0);
	if(__result){
		const char *error = lua_tostring(__L,-1);
		if(error){
			printf("error on __on_packet:%s\n",error);
		}	
	}
	lua_settop(__L,__oldtop);	
	return 0;	
}

static void on_disconnected(stream_conn_t c,int err){
	luasocket_t luasock = (luasocket_t)stream_conn_getud(c);
	luaRef_t  *obj = &luasock->luaObj;
	const char *error = LuaCallTabFuncS(*obj,"__on_disconnected","i",err);
	if(error){
		printf("error on __on_disconnected:%s\n",error);
	}
	release_luaRef(&luasock->luaObj);
	free(luasock);		
}


static int luasocket_establish(lua_State *L){
	luasocket_t luasock = (luasocket_t)lua_touserdata(L,1);
	uint32_t    recvbuf_size = lua_tointeger(L,2);
	decoder*    _decoder = (decoder*)lua_touserdata(L,3);
	recvbuf_size = size_of_pow2(recvbuf_size);
    if(recvbuf_size < 1024) recvbuf_size = 1024;
	stream_conn_t conn = new_stream_conn(luasock->sock,recvbuf_size,_decoder);
	stream_conn_associate(g_engine,conn,on_packet,on_disconnected);	
	luasock->type = _STREAM_CONN;
	luasock->streamconn = conn;
	stream_conn_setud(conn,luasock);
	return 0;
}

static void cb_connect(handle_t s,int err,void *ud,kn_sockaddr *_)
{
	((void)_);
	luasocket_t luasock = kn_sock_getud(s);
	luaRef_t  *obj = &luasock->luaObj;
	const char*error = LuaCallTabFuncS(*obj,"___cb_connect","pi",luasock->sock,err);
	if(error){
		printf("error on ___cb_connect:%s\n",error);
	}
}

static int luasocket_connect(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type != _SOCKET){
		lua_pushstring(L,"invaild socket");
		return 1;
	}
	
	if(!luaL_checkstring(L,2)){
		lua_pushstring(L,"invalid ip");
		return 1;
	}
	
	if(!luaL_checkunsigned(L,3)){
		lua_pushstring(L,"invalid port");
		return 1;		
	}
	const char *ip = lua_tostring(L,2);
	int port = lua_tointeger(L,3);	
	kn_sockaddr host;
	kn_addr_init_in(&host,ip,port);	
	if(0 != kn_sock_connect(g_engine,luasock->sock,&host,NULL,cb_connect,NULL)){
		lua_pushstring(L,"connect error");
	}else
		lua_pushnil(L);
	kn_sock_setud(luasock->sock,luasock);
	return 1;
}


static void on_new_conn(handle_t s,void* ud){
	luasocket_t luasock = (luasocket_t)ud;
	luaRef_t  *obj = &luasock->luaObj;
	const char*error = LuaCallTabFuncS(*obj,"__on_new_connection","p",s);
	if(error){
		printf("error on __on_new_connection:%s\n",error);
		return;
	}	
}


static int luasocket_listen(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type != _SOCKET){
		lua_pushstring(L,"invaild socket");
		return 1;
	}
		
	if(!luaL_checkstring(L,2)){
		lua_pushstring(L,"invalid ip");
		return 1;
	}	
	if(!luaL_checkunsigned(L,3)){
		lua_pushstring(L,"invalid port");
		return 1;		
	}	
	const char *ip = lua_tostring(L,2);
	int port = lua_tointeger(L,3);	
	kn_sockaddr local;
	kn_addr_init_in(&local,ip,port);		
	if(0 != kn_sock_listen(g_engine,luasock->sock,&local,on_new_conn,luasock)){
		lua_pushstring(L,"listen error");
	}else
		lua_pushnil(L);
	return 1;		
}

static int luasocket_close(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type == _SOCKET){
		kn_close_sock(luasock->sock);
		release_luaRef(&luasock->luaObj);
		free(luasock);				
	}else{
		stream_conn_close(luasock->streamconn);
	}
	return 0;
}

static int luasocket_tostring(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	char buf[16];
	sprintf(buf,"%x",luasock);
	lua_pushstring(L,buf);
	return 1;
}

static int luasocket_send(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type != _STREAM_CONN){
		lua_pushstring(L,"invaild socket");
		return 1;
	}
	
	lua_packet_t pk = lua_getluapacket(L,2);
	if(pk->_packet->type == RPACKET){
		lua_pushstring(L,"invaild data");
		return 1;				
	} 
	if(0 != stream_conn_send(luasock->streamconn,(packet_t)pk->_packet))
		lua_pushstring(L,"send error");
	else
		lua_pushnil(L);
	pk->_packet = NULL;
	return 1;
}


#define REGISTER_CONST(___N) do{\
		lua_pushstring(L, #___N);\
		lua_pushinteger(L, ___N);\
		lua_settable(L, -3);\
}while(0)

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

int lua_new_rawdecoder(lua_State *L){
	lua_pushlightuserdata(L,new_rawpk_decoder());
	return 1;
}

int lua_new_rpkdecoder(lua_State *L){
	uint32_t maxpacket_size = 4096;
	if(lua_gettop(L) == 1)
		maxpacket_size = lua_tointeger(L,1);
	lua_pushlightuserdata(L,new_rpk_decoder(maxpacket_size));
	return 1;
}		

void reg_luasocket(lua_State *L){
	lua_newtable(L);
		
	REGISTER_CONST(AF_INET);
	REGISTER_CONST(AF_INET6);
	REGISTER_CONST(AF_LOCAL);
	REGISTER_CONST(IPPROTO_TCP);
	REGISTER_CONST(IPPROTO_UDP);
	REGISTER_CONST(SOCK_STREAM);
	REGISTER_CONST(SOCK_DGRAM);

	REGISTER_FUNCTION("new1",luasocket_new1);
	REGISTER_FUNCTION("new2",luasocket_new2);	
	REGISTER_FUNCTION("establish",luasocket_establish);	
	REGISTER_FUNCTION("close",luasocket_close);	
	REGISTER_FUNCTION("send",luasocket_send);
	REGISTER_FUNCTION("listen",luasocket_listen);	
	REGISTER_FUNCTION("connect",luasocket_connect);
	REGISTER_FUNCTION("tostring",luasocket_tostring);	
	REGISTER_FUNCTION("rawdecoder",lua_new_rawdecoder);	
	REGISTER_FUNCTION("rpkdecoder",lua_new_rpkdecoder);	
	
	lua_setglobal(L,"CSocket");
	reg_luapacket(L);	
	g_L = L;
}



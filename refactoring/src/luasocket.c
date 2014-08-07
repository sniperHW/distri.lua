#include "luasocket.h"
#include "stream_conn.h"

__thread engine_t g_engine;


enum{
	_SOCKET = 1,
	_STREAM_CONN = 2,
};

typedef struct {
	handle_t      sock;
	stream_conn_t streamconn;
	int           type;
	luaTabRef_t   luaObj;	
}*luasocket_t;


static int luasocket_new1(lua_State *L){
	luaTabRef_t obj = create_luaTabRef(L,1);
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
	luaTabRef_t obj = create_luaTabRef(L,1);	
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
	luaTabRef_t  *obj = &luasock->luaObj;	
	const char *msg = rawpacket_data((rawpacket_t)p,NULL);
	const char *error;
	if((error = CallLuaTabFunc1(*obj,"__on_packet",0,lua_pushstring(obj->L,msg)))){
		printf("error on __on_packet:%s\n",error);
	}
	return 1;	
}

static void on_disconnected(stream_conn_t c,int err){
	luasocket_t luasock = (luasocket_t)stream_conn_getud(c);
	luaTabRef_t  *obj = &luasock->luaObj;
	const char *error;
	if((error = CallLuaTabFunc1(*obj,"__on_disconnected",0,lua_pushinteger(obj->L,err)))){
		printf("error on __on_disconnected:%s\n",error);
	}
	release_luaTabRef(&luasock->luaObj);
	free(luasock);		
}


static int luasocket_establish(lua_State *L){
	luasocket_t luasock = (luasocket_t)lua_touserdata(L,1);
	uint32_t     max_packet_size = lua_tointeger(L,2);
	max_packet_size = size_of_pow2(max_packet_size);
    if(max_packet_size < 1024) max_packet_size = 1024;
	stream_conn_t conn = new_stream_conn(luasock->sock,max_packet_size,RAWPACKET);
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
	luaTabRef_t  *obj = &luasock->luaObj;
	const char*error;
	if((error = CallLuaTabFunc1(*obj,"__cb_connect",0,lua_pushinteger(obj->L,err)))){
		printf("error on __cb_connect:%s\n",error);
	}
}

static int luasocket_connect(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type != _SOCKET){
		lua_pushstring(L,"invaild socket");
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
	return 1;
}


static void on_new_conn(handle_t s,void* _){
	((void)_);
	luasocket_t luasock = (luasocket_t)kn_sock_getud(s);
	luaTabRef_t  *obj = &luasock->luaObj;
	const char*error;
	if((error = CallLuaTabFunc1(*obj,"__on_new_conn",0,lua_pushlightuserdata(obj->L,s)))){
		printf("error on __on_new_conn:%s\n",error);
		return;
	}	
}


static int luasocket_listen(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	int port = lua_tointeger(L,3);	
	kn_sockaddr local;
	kn_addr_init_in(&local,ip,port);		
	if(0 != kn_sock_listen(g_engine,luasock->sock,&local,on_new_conn,NULL)){
		lua_pushstring(L,"listen error");
	}else
		lua_pushnil(L);
	return 1;		
}

static int luasocket_close(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type == _SOCKET){
		kn_close_sock(luasock->sock);
		release_luaTabRef(&luasock->luaObj);
		free(luasock);				
	}else{
		stream_conn_close(luasock->streamconn);
	}
	return 0;
}

static int luasocket_send(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type != _STREAM_CONN){
		lua_pushstring(L,"invaild socket");
		return 1;
	}
	
	const char *msg = lua_tostring(L,2);
	rawpacket_t pk = rawpacket_create2((void*)msg,strlen(msg)+1);
	if(0 != stream_conn_send(luasock->streamconn,(packet_t)pk))
		lua_pushstring(L,"send error");
	else
		lua_pushnil(L);
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

void reg_luasocket(lua_State *L){
	lua_newtable(L);
		
	REGISTER_CONST(AF_INET);
	REGISTER_CONST(AF_INET6);
	REGISTER_CONST(AF_LOCAL);
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
	lua_setglobal(L,"luasocket");
}



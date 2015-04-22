#include "luasocket.h"
#include "luapacket.h"
#include "push_callback.h"
#include "log.h"
#include "listener.h"
#include "kn_thread_mailbox.h"

extern engine_t g_engine;

enum{
	_SOCKET = 1,
	_STREAM_CONN = 2,
	_DATAGRAM = 3,
};

typedef struct{
	luaPushFunctor base;
	packet_t p;
}stPushPacket;

static void PushPacket(lua_State *L,luaPushFunctor_t _){
	stPushPacket *self = (stPushPacket*)_;
	push_luapacket(L,self->p);
}

typedef struct{
	luaPushFunctor base;
	kn_sockaddr *addr;	
}stPushAddr;

static void PushAddr(lua_State *L,luaPushFunctor_t _){
	stPushAddr *self = (stPushAddr*)_;
	if(!self->addr)
		lua_pushnil(L);
	else{
		if(self->addr->addrtype == AF_INET){
			lua_newtable(L);
			char ip[32];
			inet_ntop(AF_INET,&self->addr->in.sin_addr,ip,32);
			int    port = ntohs(self->addr->in.sin_port);
			lua_pushinteger(L,AF_INET);
			lua_rawseti(L,-2,1);		
			lua_pushstring(L,ip);
			lua_rawseti(L,-2,2);
			lua_pushinteger(L,port);
			lua_rawseti(L,-2,3);
		}else if(self->addr->addrtype == AF_LOCAL){
			lua_newtable(L);
			char path[256];
			strncpy(path,self->addr->un.sun_path,sizeof(path)-1);
			lua_pushinteger(L,AF_LOCAL);
			lua_rawseti(L,-2,1);		
			lua_pushstring(L,path);
			lua_rawseti(L,-2,2);			
		}else
			lua_pushnil(L);
	}
}

static void on_datagram(struct datagram *d,packet_t p,kn_sockaddr *from){
	luasocket_t luasock = (luasocket_t)datagram_getud(d);
	luaRef_t  *obj = &luasock->luaObj;
	stPushPacket st1;
	st1.p = clone_packet(p);
	st1.base.Push = PushPacket;

	stPushAddr st2;
	st2.addr = from;
	st2.base.Push = PushAddr;

	const char *error = push_obj_callback(obj->L,"srff","__on_packet",*obj,&st1,&st2);
	if(error){
		SYS_LOG(LOG_ERROR,"error on __on_packet:%s\n",error);	
	}	
}

static void destroy_luasocket(void *_){
	luasocket_t lsock = (luasocket_t)_;
	release_luaRef(&lsock->luaObj);
	free(lsock);	
}

static int luasocket_new1(lua_State *L){
	luaRef_t obj = toluaRef(L,1);
	int domain = lua_tointeger(L,2);
	int type = lua_tointeger(L,3);
	luasocket_t luasock;
	handle_t sock;
	if(type == SOCK_STREAM){
		sock = kn_new_sock(domain,type,IPPROTO_TCP);	
		if(!sock){
			lua_pushnil(L);
			return 1;
		}
		luasock = calloc(1,sizeof(*luasock));
		luasock->type = _SOCKET;
		kn_sock_setud(sock,luasock);	
	}else if(type == SOCK_DGRAM){
		uint32_t    recvbuf_size = lua_tointeger(L,4);
		decoder*    _decoder = (decoder*)lua_touserdata(L,5);
		luasock = calloc(1,sizeof(*luasock));
		luasock->datagram = new_datagram(domain,recvbuf_size,_decoder);
		luasock->type = _DATAGRAM;
		sock = datagram_gethandle(luasock->datagram);				
		datagram_setud(luasock->datagram,luasock,destroy_luasocket);
		datagram_associate(g_engine,luasock->datagram,on_datagram); 	
	}else{
		lua_pushnil(L);
		return 1;
	}
	luasock->sock = sock;
	luasock->luaObj = obj;	
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

static void  on_packet(connection_t c,packet_t p){
	luasocket_t luasock = (luasocket_t)connection_getud(c);
	luaRef_t  *obj = &luasock->luaObj;
	stPushPacket st;
	st.p = clone_packet(p);
	st.base.Push = PushPacket;
	const char *error = push_obj_callback(obj->L,"srf","__on_packet",*obj,&st);
	if(error){
		SYS_LOG(LOG_ERROR,"error on __on_packet:%s\n",error);	
	}	
}

static int luasocket_close(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type == _SOCKET){
		if(luasock->listening){
			listener_close_listen(luasock);
		}else{
			kn_close_sock(luasock->sock);
			destroy_luasocket(luasock);
		}						
	}else if(luasock->type == _STREAM_CONN){
		connection_close(luasock->streamconn);
		refobj_dec((refobj*)luasock->streamconn);
	}else{
		datagram_close(luasock->datagram);
		refobj_dec((refobj*)luasock->datagram);
	}
	return 0;
}

static void on_disconnected(connection_t c,int err){
	//printf("on_disconnected\n");
	luasocket_t luasock = (luasocket_t)connection_getud(c);
	luaRef_t  *obj = &luasock->luaObj;
	const char *error = push_obj_callback(obj->L,"sri","__on_disconnected",*obj,err);
	if(error){
		SYS_LOG(LOG_ERROR,"error on __on_disconnected:%s\n",error);	
	}	
}

int mask_http_decode;
void on_http_cb(connection_t c,packet_t p);
static int luasocket_establish(lua_State *L){
	luasocket_t luasock = (luasocket_t)lua_touserdata(L,1);
	uint32_t    recvbuf_size = lua_tointeger(L,2);
	decoder*    _decoder = (decoder*)lua_touserdata(L,3);
	recvbuf_size = size_of_pow2(recvbuf_size);
    	if(recvbuf_size < 1024) recvbuf_size = 1024;
	connection_t conn = new_connection(luasock->sock,recvbuf_size,_decoder);
	
	if(_decoder && _decoder->mask == mask_http_decode)
		connection_associate(g_engine,conn,on_http_cb,on_disconnected);
	else
		connection_associate(g_engine,conn,on_packet,on_disconnected);
	refobj_inc((refobj*)conn);
	luasock->type = _STREAM_CONN;
	luasock->streamconn = conn;
	connection_setud(conn,luasock,destroy_luasocket);
	return 0;
}

static void cb_connect(handle_t s,void *remoteaddr,int _ ,int err)
{
	((void)_);
	luasocket_t luasock = kn_sock_getud(s);
	luaRef_t  *obj = &luasock->luaObj;
	const char *error = push_obj_callback(obj->L,"sri","___cb_connect",*obj,err);
	if(error){
		SYS_LOG(LOG_ERROR,"error on ___cb_connect:%s\n",error);	
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
	
	if(!luaL_checkinteger(L,3)){
		lua_pushstring(L,"invalid port");
		return 1;		
	}
	const char *ip = lua_tostring(L,2);
	int port = lua_tointeger(L,3);	
	kn_sockaddr host;
	kn_addr_init_in(&host,ip,port);
	int ret = kn_sock_connect(luasock->sock,&host,NULL);
	if(ret > 0){
		lua_pushnil(L);
		lua_pushinteger(L,ret);
	}else if(ret == 0){
		kn_engine_associate(g_engine,luasock->sock,cb_connect);
		lua_pushnil(L);
		lua_pushinteger(L,ret);		
	}else{
		lua_pushstring(L,"connect error");
		lua_pushnil(L);
	}	
	kn_sock_setud(luasock->sock,luasock);
	return 2;
}

static int luasocket_listen(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	if(luasock->type == _SOCKET){
		if(!luaL_checkstring(L,2)){
			lua_pushstring(L,"invalid ip");
			return 1;
		}	
		if(!luaL_checkinteger(L,3)){
			lua_pushstring(L,"invalid port");
			return 1;		
		}	
		const char *ip = lua_tostring(L,2);
		int port = lua_tointeger(L,3);	
		kn_sockaddr local;
		kn_addr_init_in(&local,ip,port);
		int ret = listener_listen(luasock,&local);
		if(0 != ret){
			lua_pushstring(L,"listen error");
		}else{
			luasock->listening = 1;
			lua_pushnil(L);
		}
	}else if(luasock->type == _DATAGRAM){
		if(!luaL_checkstring(L,2)){
			lua_pushstring(L,"invalid ip");
			return 1;
		}	
		if(!luaL_checkinteger(L,3)){
			lua_pushstring(L,"invalid port");
			return 1;		
		}	
		const char *ip = lua_tostring(L,2);
		int port = lua_tointeger(L,3);	
		kn_sockaddr local;
		kn_addr_init_in(&local,ip,port);
		if(0 == kn_sock_listen(datagram_gethandle(luasock->datagram),&local)){
			lua_pushnil(L);
		}else{
			lua_pushstring(L,"listen error");
		}		
	}		
	return 1;		
}

static int luasocket_tostring(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	char buf[16];
	sprintf(buf,"%lx",(long unsigned int)luasock);
	lua_pushstring(L,buf);
	return 1;
}

static int lua_getaddr(lua_State *L,int idx,kn_sockaddr *addr){
	if(!lua_istable(L,idx))
		return -1;
	lua_rawgeti(L,idx,1);
	int domain = lua_tointeger(L,-1);
	if(domain == AF_INET){
		lua_rawgeti(L,idx,2);
		const char *ip = lua_tostring(L,-1);
		lua_rawgeti(L,idx,3);
		uint32_t port = lua_tointeger(L,-1);
		kn_addr_init_in(addr,ip,port);
	}else if(domain == AF_LOCAL){
		lua_rawgeti(L,idx,2);
		const char *path = lua_tostring(L,-1);
		kn_addr_init_un(addr,path);
	}else{
		return -1;
	}
	return 0;
}

static void send_complete_callback(connection_t c){
	luasocket_t luasock = (luasocket_t)connection_getud(c);
	luaRef_t  *obj = &luasock->luaObj;
	const char *error = push_obj_callback(obj->L,"sr","__send_complete",*obj);
	if(error){
		SYS_LOG(LOG_ERROR,"error on __send_complete:%s\n",error);	
	}
} 

static  int _luasocket_stream_send(lua_State *L,void (*complete_callback)(connection_t)){
	luasocket_t luasock = lua_touserdata(L,1);
	int type = luasock->type;
	if(type != _STREAM_CONN){
		lua_pushstring(L,"invaild socket");
		return 1;
	}

	lua_packet_t pk = lua_getluapacket(L,2);
	if(pk->_packet->type == RPACKET){
		lua_pushstring(L,"invaild data");
		return 1;				
	}
	if(0 != connection_send(luasock->streamconn,(packet_t)pk->_packet,complete_callback))
		lua_pushstring(L,"send error");
	else
		lua_pushnil(L);
	pk->_packet = NULL;
	return 1;	
}

static int luasocket_stream_send(lua_State *L){
	return _luasocket_stream_send(L,NULL);
}

static int luasocket_stream_syncsend(lua_State *L){
	return _luasocket_stream_send(L,send_complete_callback);
}

static int luasocket_datagram_send(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	int type = luasock->type;
	if(type != _DATAGRAM){
		lua_pushstring(L,"invaild socket");
		return 1;
	}
	lua_packet_t pk = lua_getluapacket(L,2);
	if(pk->_packet->type == RPACKET){
		lua_pushstring(L,"invaild data");
		return 1;				
	}
	int ret = -1;
	kn_sockaddr to;
	if(0 == lua_getaddr(L,3,&to)){
		ret = datagram_send(luasock->datagram,(packet_t)pk->_packet,&to);
	}
	if(ret >= 0)
		lua_pushnil(L);
	else
		lua_pushstring(L,"send error");
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

int lua_new_datagram_rawdecoder(lua_State *L){
	lua_pushlightuserdata(L,new_datagram_rawpk_decoder());
	return 1;
}

int lua_new_datagram_rpkdecoder(lua_State *L){
	lua_pushlightuserdata(L,new_datagram_rpk_decoder());
	return 1;
}

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	li_msg_t _msg = (li_msg_t)mail;
	if(_msg->_msg_type == MSG_CLOSED){
		luasocket_t luasock = (luasocket_t)_msg->luasock;
		destroy_luasocket(luasock);
	}else if(_msg->_msg_type == MSG_CONNECTION){
		luasocket_t luasock = (luasocket_t)_msg->luasock;
		luaRef_t  *obj = &luasock->luaObj;
		const char *error = push_obj_callback(obj->L,"srp","__on_new_connection",*obj,_msg->ud);
		if(error){
			SYS_LOG(LOG_ERROR,"error on __on_new_connection:%s\n",error);			
		}				
	}
}

kn_thread_mailbox_t mainmailbox;		

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
	REGISTER_FUNCTION("stream_send",luasocket_stream_send);
	REGISTER_FUNCTION("stream_syncsend",luasocket_stream_syncsend);	
	REGISTER_FUNCTION("datagram_send",luasocket_datagram_send);
	REGISTER_FUNCTION("listen",luasocket_listen);	
	REGISTER_FUNCTION("connect",luasocket_connect);
	REGISTER_FUNCTION("tostring",luasocket_tostring);	
	REGISTER_FUNCTION("rawdecoder",lua_new_rawdecoder);	
	REGISTER_FUNCTION("rpkdecoder",lua_new_rpkdecoder);
	REGISTER_FUNCTION("datagram_rawdecoder",lua_new_datagram_rawdecoder);	
	REGISTER_FUNCTION("datagram_rpkdecoder",lua_new_datagram_rpkdecoder);	
	
	lua_setglobal(L,"CSocket");
	reg_luapacket(L);
	mainmailbox = kn_setup_mailbox(g_engine,on_mail);
}



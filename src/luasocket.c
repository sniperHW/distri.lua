#include "luasocket.h"
#include "luapacket.h"
#include "push_callback.h"
#include "log.h"
#include "listener.h"
#include "kn_thread_mailbox.h"

extern engine_t g_engine;
//static __thread lua_State *g_L;
enum{
	_SOCKET = 1,
	_STREAM_CONN = 2,
};

static int luasocket_new1(lua_State *L){
	luaRef_t obj = toluaRef(L,1);
	int domain = lua_tointeger(L,2);
	//int type = lua_tointeger(L,3);
	//int protocal = lua_tointeger(L,4);
	handle_t sock = kn_new_sock(domain,SOCK_STREAM,IPPROTO_TCP);	
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

static void destroy_luasocket(void *_){
	luasocket_t lsock = (luasocket_t)_;
	release_luaRef(&lsock->luaObj);
	free(lsock);	
}

typedef struct{
	luaPushFunctor base;
	packet_t p;
}stPushPacket;

static void PushPacket(lua_State *L,luaPushFunctor_t _){
	stPushPacket *self = (stPushPacket*)_;
	push_luapacket(L,self->p);
	//packet_ref_inc(self->p);
}

static void  on_packet(connection_t c,packet_t p){
	luasocket_t luasock = (luasocket_t)connection_getud(c);
	luaRef_t  *obj = &luasock->luaObj;
	stPushPacket st;
	st.p = clone_packet(p);//(packet_t)rpk_copy_create(p);
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
	}else{
		connection_close(luasock->streamconn);
		refobj_dec((refobj*)luasock->streamconn);
	}
	return 0;
}

static void on_disconnected(connection_t c,int err){
	printf("on_disconnected\n");
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
	connection_associate(g_engine,conn,_decoder->mask == mask_http_decode?on_http_cb:on_packet,on_disconnected);	
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
	const char *error = push_obj_callback(obj->L,"srpi","___cb_connect",*obj,luasock->sock,err);
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
	
	if(!luaL_checkunsigned(L,3)){
		lua_pushstring(L,"invalid port");
		return 1;		
	}
	const char *ip = lua_tostring(L,2);
	int port = lua_tointeger(L,3);	
	kn_sockaddr host;
	kn_addr_init_in(&host,ip,port);
	int ret = kn_sock_connect(luasock->sock,&host,NULL);
	if(ret > 0){//kn_sock_connect(p,c,&remote,NULL)){
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
	int ret = listener_listen(luasock,&local);
	if(0 != ret){
		lua_pushstring(L,"listen error");
	}else{
		luasock->listening = 1;
		lua_pushnil(L);
	}		
	/*if(0 != kn_sock_listen(g_engine,luasock->sock,&local,on_new_conn,luasock)){
		lua_pushstring(L,"listen error");
	}else
		lua_pushnil(L);*/
	return 1;		
}

static int luasocket_tostring(lua_State *L){
	luasocket_t luasock = lua_touserdata(L,1);
	char buf[16];
	sprintf(buf,"%lx",(long unsigned int)luasock);
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
	if(0 != connection_send(luasock->streamconn,(packet_t)pk->_packet))
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

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	li_msg_t _msg = (li_msg_t)mail;
	if(_msg->_msg_type == MSG_CLOSED){
		luasocket_t luasock = (luasocket_t)_msg->luasock;
		destroy_luasocket(luasock);
		//kn_close_sock(luasock->sock);
		//send2main(new_msg(MSG_CLOSED,_msg->ud));
	}else if(_msg->_msg_type == MSG_CONNECTION){
		luasocket_t luasock = (luasocket_t)_msg->luasock;
		luaRef_t  *obj = &luasock->luaObj;
		const char *error = push_obj_callback(obj->L,"srp","__on_new_connection",*obj,_msg->ud);
		if(error){
			SYS_LOG(LOG_ERROR,"error on __on_new_connection:%s\n",error);			
		}				
	}
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
	kn_setup_mailbox(g_engine,on_mail);




	//g_L = L;
}



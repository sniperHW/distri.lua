#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "lua_util.h"
#include "kendynet.h"
#include "kn_time.h"

static kn_proactor_t g_proactor = NULL;
static lua_State     g_L = NULL;

static void sig_int(int sig){
	recv_sigint = 1;
}

enum{
	lua_socket_close = 1,
	lua_socket_writeclose,
	lua_socket_waitclose,//等send_list中的数据发送完成自动close
};

typedef struct lua_socket{
	kn_ref      ref;	
	st_io       send_overlap;
	st_io       recv_overlap;
	struct      iovec wrecvbuf[1];
	char        recvbuf[65536];
	struct      iovec wsendbuf[512];
	uint8_t     status;	
	kn_list     send_list;	
	kn_socket_t sock;
	luaObject_t callbackObj;//存放lua回调函数
}*lua_socket_t;

typedef struct sendbuf{
	kn_sockaddr remote;
	char *buf;
	int   size;
	int   index;
}sendbuf;

void luasocket_destroyer(void *ptr){
	lua_socket_t l = (lua_socket_t)ptr;
	kn_closesocket(l->sock);
	release_luaObj(l->callbackObj);
	while(kn_list_size(&l->send_list)){
		sendbuf *buf = kn_list_pop(&l->send_list);
		free(buf->buf);
		free(buf);
	}
}

lua_socket_t new_luasocket(kn_socket_t sock,luaObject_t callbackObj){
	lua_socket_t l = calloc(1,sizeof(*l));
	l->sock = sock;
	l->callbackObj = callbackObj;
	kn_ref_init(&l->ref,luasocket_destroyer);
	return l;
}



static void luasocket_close(lua_socket_t l)
{
	if(l->status == 0 && kn_list_size(&l->send_list))
	{
		l->status = lua_socket_waitclose;
		return;		
	}	
	l->status = lua_socket_close;
	kn_ref_release(&l->ref);
}

static void luasocket_post_recv(lua_socket_t l){
	if(kn_socket_get_type(l->socket) == STREAM_SOCKET){
		l->wrecvbuf[0].iov_base = s->recvbuf;
		l->wrecvbuf[0].iov_len = 65535;
		l->recv_overlap.iovec_count = 1;
		l->recv_overlap.iovec = l->wrecvbuf;
		kn_post_recv(l->sock,&l->recv_overlap);
	}
}

static void luasocket_post_send(lua_socket_t l){
	if(kn_socket_get_type(l->socket) == STREAM_SOCKET){
		int c = 0;
		sendbuf *buf = (sendbuf*)kn_list_head(l->send_list);
		while(c < 512 && buf){
			l->wsendbuf[c].iov_base = buf->buf+buf->index;
			l->wsendbuf[c].iov_len = buf->size;
		}
		l->send_overlap.iovec_count = c;
		l->send_overlap.iovec = l->wsendbuf;
		kn_post_send(l->sock,&l->send_overlap);		
	}
}

static void stream_recv_finish(lua_socket_t l,st_io *io,int32_t bytestransfer,int32_t err)
{
	if(bytestransfer <= 0){
		CALL_OBJ_FUNC4(l->callbackObj,"recvfinish",0,
		               lua_lightuserdata(l->callbackObj->L,l),
		               lua_pushnil(l->callbackObj->L),
		               lua_pushnumber(l->callbackObj->L,err)
		               );       
	}else{
		l->recvbuf[bytestransfer] = 0;
		CALL_OBJ_FUNC4(l->callbackObj,"recvfinish",0,
			   lua_lightuserdata(l->callbackObj->L,l),
			   lua_pushstring(l->callbackObj->L,l->recvbuf[bytestransfer]),
			   lua_pushnumber(l->callbackObj->L,err)
			   );
		if(l->status != lua_socket_close){	   			   
			//发起新的接收
			lua_socket_post_recv(l);
		}	     
	}
}

static void stream_send_finish(lua_socket_t l,st_io *io,int32_t bytestransfer,int32_t err)
{
	if(bytestransfer <= 0)
		l->status = lua_socket_writeclose;
	else{
		while(bytestransfer > 0){
			sendbuf *buf = (sendbuf*)kn_list_head(l->send_list);
			int size = buf->size - buf->index;
			if(size <= bytestransfer)
			{
				bytestransfer -= size;
				kn_list_pop(l->send_list);
				free(buf);
			}else{
				buf->index += bytestransfer;
				break;
			}
		}
		if(kn_list_size(l->send_list)){
			lua_socket_post_send(l);
		}else if(l->status == lua_socket_waitclose)
			//关闭lua_socket
			luasocket_close(l);
		}
	}
}

static void stream_transfer_finish(kn_socket_t s,st_io *io,int32_t bytestransfer,int32_t err)
{    
	uint32_t luaidentity;
    lua_socket_t l = (lua_socket_t)kn_socket_getud(s);
    kn_ref_acquire(&l->ref);
    //防止l在callback中被释放
    if(!io){
		CALL_OBJ_FUNC4(l->callbackObj,"recvfinish",0,
		               lua_lightuserdata(l->callbackObj->L,l),
		               lua_pushnil(l->callbackObj->L),
		               lua_pushnumber(l->callbackObj->L,err)
		               );                       		
	}else if(io == &l->send_overlap)
		stream_send_finish(l,io,bytestransfer,err);
	else if(io == &l->recv_overlap)
		stream_recv_finish(l,io,bytestransfer,err);
	kn_ref_release(&l->ref);
}


static void on_connect(kn_socket_t s,struct kn_sockaddr *remote,void *ud,int err)
{	
	
}

static void on_accept(kn_socket_t s,void *ud){
	l = new_luasocket(s,(luaObject_t)ud);
	kn_socket_setud(s,l);
	CALL_OBJ_FUNC1(l->callbackObj,"onaccept",0,lua_lightuserdata(l->callbackObj->L,l));   		
}


int lua_close(lua_State *L){
	lua_socket_t l = lua_tolightuserdata(L,-1);
	luasocket_close(l);
	return 0;
}

int lua_asyn_connect(lua_State *L){
	return 0;
}

int lua_connect(lua_State *L){
	return 0;
}

int lua_listen(lua_State *L){
	int proto = lua_tonumber(L,-1);
	int sock_type = lua_tonumber(L,-2);
	luaObject_t addr = create_luaObj(L,-3);
	kn_sockaddr addr_local;
	int type = GET_OBJ_FIELD(addr,"type",int,lua_tonumber);
	if(type == AF_INET){
		kn_addr_init_in(&addr_local,
						GET_OBJ_FIELD(addr,"ip4",const char*,lua_tostring),
						GET_OBJ_FIELD(addr,"port",int,lua_number));	
		kn_socket_t l = kn_listen(g_proactor,proto,sock_type,&addr_local,on_accept,(void*)create_luaObj(L,-4));
		lua_pushlightuserdata(L,l);
		return 1;			
	}
	lua_pushnil(L);
	return 1;
}

int lua_send(lua_State *L){
	lua_socket_t l  = lua_tolightuserdata(L,-1);
	luaObject_t  addr_remote = create_luaObj(L,-2);
	const char * msg = lua_tostring(L,-3);
	
}

void RegisterNet(lua_State *L){  
    
	lua_register(L,"clisten",&lua_listen);
	lua_register(L,"cconnect",&lua_connect);
	lua_register(L,"casyn_connect",&lua_asyn_connect);   
    lua_register(L,"csend",&lua_send);
	lua_register(L,"close",&lua_close);   
	g_L = L;
	InitNetSystem();
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
    g_proactor = kn_new_proactor();
    printf("load c function finish\n");  
} 

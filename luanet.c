#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "lua_util.h"
#include "kendynet.h"
#include "kn_time.h"
#include "kn_ref.h"

static kn_proactor_t g_proactor = NULL;
static lua_State*    g_L = NULL;
static int           recv_sigint = 0;

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
		sendbuf *buf = (sendbuf*)kn_list_pop(&l->send_list);
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
	if(kn_socket_get_type(l->sock) == STREAM_SOCKET){
		l->wrecvbuf[0].iov_base = l->recvbuf;
		l->wrecvbuf[0].iov_len = 65535;
		l->recv_overlap.iovec_count = 1;
		l->recv_overlap.iovec = l->wrecvbuf;
		kn_post_recv(l->sock,&l->recv_overlap);
	}
}

static void luasocket_post_send(lua_socket_t l){
	if(kn_socket_get_type(l->sock) == STREAM_SOCKET){
		int c = 0;
		sendbuf *buf = (sendbuf*)kn_list_head(&l->send_list);
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
	//callbackObj只是一个普通的表所以不能直接使用CALL_OBJ_FUNC
	lua_rawgeti(l->callbackObj->L,LUA_REGISTRYINDEX,l->callbackObj->rindex);
	lua_pushstring(l->callbackObj->L,"recvfinish");
	lua_gettable(l->callbackObj->L,-2);
	lua_pushlightuserdata(l->callbackObj->L,l);
	if(bytestransfer <= 0){
		lua_pushnil(l->callbackObj->L);   
	}else{
		l->recvbuf[bytestransfer] = 0;
		lua_pushstring(l->callbackObj->L,l->recvbuf);
	}
	lua_pushnumber(l->callbackObj->L,err);
	lua_pcall(l->callbackObj->L,3,0,0);
	lua_pop(l->callbackObj->L,1);
	if(bytestransfer > 0 && l->status != lua_socket_close)
		luasocket_post_recv(l);
}

static void stream_send_finish(lua_socket_t l,st_io *io,int32_t bytestransfer,int32_t err)
{
	if(bytestransfer <= 0)
		l->status = lua_socket_writeclose;
	else{
		while(bytestransfer > 0){
			sendbuf *buf = (sendbuf*)kn_list_head(&l->send_list);
			int size = buf->size - buf->index;
			if(size <= bytestransfer)
			{
				bytestransfer -= size;
				kn_list_pop(&l->send_list);
				free(buf);
			}else{
				buf->index += bytestransfer;
				break;
			}
		}
		if(kn_list_size(&l->send_list)){
			luasocket_post_send(l);
		}else if(l->status == lua_socket_waitclose)
			//关闭lua_socket
			luasocket_close(l);
	}
}

static void stream_transfer_finish(kn_socket_t s,st_io *io,int32_t bytestransfer,int32_t err)
{   
	printf("stream_transfer_finish\n"); 
    lua_socket_t l = (lua_socket_t)kn_socket_getud(s);
    kn_ref_acquire(&l->ref);
    //防止l在callback中被释放
    if(!io){
		lua_rawgeti(l->callbackObj->L,LUA_REGISTRYINDEX,l->callbackObj->rindex);
		lua_pushstring(l->callbackObj->L,"recvfinish");
		lua_gettable(l->callbackObj->L,-2);
		lua_pushlightuserdata(l->callbackObj->L,l);
		lua_pushnil(l->callbackObj->L);
		lua_pushnumber(l->callbackObj->L,err);				
		lua_pcall(l->callbackObj->L,3,0,0);
		lua_pop(l->callbackObj->L,1);                 		
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
	printf("c on_accept\n");
	lua_socket_t l = new_luasocket(s,(luaObject_t)ud);
	kn_socket_setud(s,l);
	 kn_ref_acquire(&l->ref);
	lua_rawgeti(l->callbackObj->L,LUA_REGISTRYINDEX,l->callbackObj->rindex);
	lua_pushstring(l->callbackObj->L,"onaccept");
	lua_gettable(l->callbackObj->L,-2);
	lua_pushlightuserdata(l->callbackObj->L,l);			
	lua_pcall(l->callbackObj->L,1,0,0);
	lua_pop(l->callbackObj->L,1);
	kn_ref_release(&l->ref);  		
}


int lua_closefd(lua_State *L){
	lua_socket_t l = lua_touserdata(L,1);
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
	int proto = lua_tonumber(L,1);
	int sock_type = lua_tonumber(L,2);
	luaObject_t addr = create_luaObj(L,3);
	kn_sockaddr addr_local;
	int type = GET_OBJ_FIELD(addr,"type",int,lua_tonumber);
	if(type == AF_INET){
		kn_addr_init_in(&addr_local,
						GET_OBJ_FIELD(addr,"ip",const char*,lua_tostring),
						GET_OBJ_FIELD(addr,"port",int,lua_tonumber));	
		kn_socket_t l = kn_listen(g_proactor,proto,sock_type,&addr_local,on_accept,(void*)create_luaObj(L,4));
		lua_pushlightuserdata(L,l);
		return 1;			
	}
	lua_pushnil(L);
	return 1;
}

int lua_send(lua_State *L){
	//lua_socket_t l  = lua_touserdata(L,-1);
	//luaObject_t  addr_remote = create_luaObj(L,-2);
	//const char * msg = lua_tostring(L,-3);
	return 0;
}

int lua_run(lua_State *L){
	while(!recv_sigint)
		kn_proactor_run(g_proactor,50);
	return 0;
}

int lua_bind(lua_State *L){
	lua_socket_t l = lua_touserdata(L,1);
	if(0 == kn_proactor_bind(g_proactor,l->sock,stream_transfer_finish)){
		l->callbackObj = create_luaObj(L,2);
		luasocket_post_recv(l);	
		lua_pushboolean(L,1);
	}
	else
		lua_pushboolean(L,0);
	return 1;
}

void RegisterNet(lua_State *L){  
    
    lua_getglobal(L,"_G");
	if(!lua_istable(L, -1))
	{
		lua_pop(L,1);
		lua_newtable(L);
		lua_pushvalue(L,-1);
		lua_setglobal(L,"_G");
	}
	

	lua_pushstring(L, "AF_INET");
	lua_pushinteger(L, AF_INET);
	lua_settable(L, -3);
	
	lua_pushstring(L, "AF_INET6");
	lua_pushinteger(L, AF_INET6);
	lua_settable(L, -3);
	
	lua_pushstring(L, "AF_LOCAL");
	lua_pushinteger(L, AF_LOCAL);
	lua_settable(L, -3);
	
	lua_pushstring(L, "IPPROTO_TCP");
	lua_pushinteger(L, IPPROTO_TCP);
	lua_settable(L, -3);
	
	lua_pushstring(L, "IPPROTO_UDP");
	lua_pushinteger(L, IPPROTO_UDP);
	lua_settable(L, -3);			
	
	lua_pushstring(L, "SOCK_STREAM");
	lua_pushinteger(L, SOCK_STREAM);
	lua_settable(L, -3);
	
	lua_pushstring(L, "SOCK_DGRAM");
	lua_pushinteger(L, SOCK_DGRAM);
	lua_settable(L, -3);	
	
	lua_pop(L,1);
    
	lua_register(L,"clisten",&lua_listen);
	lua_register(L,"cconnect",&lua_connect);
	lua_register(L,"casyn_connect",&lua_asyn_connect);   
    lua_register(L,"csend",&lua_send);
	lua_register(L,"close",&lua_closefd);
	lua_register(L,"run",&lua_run);
	lua_register(L,"cbind",&lua_bind);  
	g_L = L;
	kn_net_open();
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
    g_proactor = kn_new_proactor();
    printf("load c function finish\n");  
}

int main(int argc,char **argv)
{
	if(argc < 2){
		printf("usage luanet luafile\n");
	}
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	RegisterNet(L);
	if (luaL_dofile(L,argv[1])) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}	
	return 0;
} 

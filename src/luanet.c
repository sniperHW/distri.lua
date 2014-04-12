#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "lua_util.h"
#include "kendynet.h"
#include "kn_time.h"
#include "kn_ref.h"
#include "kn_list.h"

static kn_proactor_t g_proactor = NULL;
static lua_State*    g_L = NULL;
static int           recv_sigint = 0;
static int           recv_count = 0;

static void sig_int(int sig){
	recv_sigint = 1;
}

enum{
	lua_socket_close = 1,
	lua_socket_writeclose,
	lua_socket_waitclose,//等send_list中的数据发送完成自动close
};

typedef struct bytebuffer{
	struct bytebuffer *next;
	int    cap;
	int    size;
	char   data[0];
}*bytebuffer_t;

bytebuffer_t new_bytebuffer(int cap)
{
	bytebuffer_t b = calloc(1,sizeof(*b)+sizeof(char)*cap);
	b->cap = cap;
	b->size = 0;
	return b;
}

static inline int bytebuffer_read(bytebuffer_t b,uint32_t pos,int8_t *out,uint32_t size)
{
	uint32_t copy_size;
	while(size){
        if(!b) return -1;
        if(pos >= b->size) return -1;
        copy_size = b->size - pos;
		copy_size = copy_size > size ? size : copy_size;
		memcpy(out,&b->data[pos],copy_size);
		size -= copy_size;
		pos += copy_size;
		out += copy_size;
		if(pos >= b->size){
			pos = 0;
			b = b->next;
		}
	}
	return 0;
}

typedef struct lua_socket{
	kn_ref       ref;	
	st_io        send_overlap;
	st_io        recv_overlap;
	struct       iovec wrecvbuf[2];
	struct       iovec wsendbuf[512];
	uint8_t      status;	
	kn_list      send_list;
	uint32_t     unpack_size; //还未解包的数据大小
	uint32_t     unpack_pos;
	uint32_t     recv_pos;
	bytebuffer_t recv_buf;
	bytebuffer_t unpack_buf;
	kn_socket_t  sock;
	luaObject_t  callbackObj;//存放lua回调函数
}*lua_socket_t;

typedef struct sendbuf{
	kn_list_node node;
	kn_sockaddr  remote;
	int          index;
	int          size;
	char         buf[0];
}sendbuf;

void luasocket_destroyer(void *ptr){
	lua_socket_t l = (lua_socket_t)ptr;
	kn_closesocket(l->sock);
	release_luaObj(l->callbackObj);
	while(kn_list_size(&l->send_list)){
		sendbuf *buf = (sendbuf*)kn_list_pop(&l->send_list);
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
	while(l->unpack_buf){
		bytebuffer_t tmp = l->unpack_buf;
		l->unpack_buf = l->unpack_buf->next;
		free(tmp);
	}		
	l->status = lua_socket_close;
	kn_ref_release(&l->ref);
}


static void update_recv_pos(lua_socket_t c,int32_t _bytestransfer)
{
    assert(_bytestransfer >= 0);
    uint32_t bytestransfer = (uint32_t)_bytestransfer;
    uint32_t size;
	do{
		size = c->recv_buf->cap - c->recv_pos;
        size = size > bytestransfer ? bytestransfer:size;
		c->recv_buf->size += size;
		c->recv_pos += size;
		bytestransfer -= size;
		if(c->recv_pos >= c->recv_buf->cap)
		{
			if(!c->recv_buf->next)
				c->recv_buf->next = new_bytebuffer(65536);
			c->recv_buf = c->recv_buf->next;
			c->recv_pos = 0;
		}
	}while(bytestransfer);
}


static void do_callback(lua_socket_t l,char *packet,int err){
	//callbackObj只是一个普通的表所以不能直接使用CALL_OBJ_FUNC
	lua_rawgeti(l->callbackObj->L,LUA_REGISTRYINDEX,l->callbackObj->rindex);
	lua_pushstring(l->callbackObj->L,"recvfinish");
	lua_gettable(l->callbackObj->L,-2);
	if(l->callbackObj->L != g_L) lua_xmove(l->callbackObj->L,g_L,1);
	lua_pushlightuserdata(g_L,l);
	if(!packet){
		lua_pushnil(g_L);   
	}else{
		lua_pushstring(g_L,packet);
		recv_count++;
	}
	lua_pushnumber(g_L,err);
	if(0 != lua_pcall(g_L,3,0,0)){
		const char * error = lua_tostring(g_L, -1);
		printf("stream_recv_finish:%s\n",error);
		lua_pop(g_L,1);
	}
	lua_pop(g_L,1);	
}

static int unpack(lua_socket_t c)
{
	uint32_t pk_len = 0;
	uint32_t pk_total_size;
	char*    packet = NULL;
	char     pk_buf[65536];
	uint32_t pos;
	do{

		if(c->unpack_size <= sizeof(uint32_t))
			return 1;
		bytebuffer_read(c->unpack_buf,c->unpack_pos,(int8_t*)&pk_len,sizeof(pk_len));
		pk_total_size = pk_len+sizeof(pk_len);
		if(pk_total_size > c->unpack_size)
			return 1;
		
		if(pk_len < 65536)
			packet = pk_buf;
		else 
			packet = calloc(1,sizeof(char)*pk_len);
		pos = 0;
		//调整unpack_buf和unpack_pos
		do{
			uint32_t size = c->unpack_buf->size - c->unpack_pos;
			size = pk_total_size > size ? size:pk_total_size;
			memcpy(&c->unpack_buf->data[c->unpack_pos],packet+pos,size);
			pos += size;
			c->unpack_pos  += size;
			pk_total_size  -= size;
			c->unpack_size -= size;
			if(c->unpack_pos >= c->unpack_buf->cap)
			{
				assert(c->unpack_buf->next);
				bytebuffer_t tmp = c->unpack_buf;
				c->unpack_pos = 0;
				c->unpack_buf = c->unpack_buf->next;
				free(tmp);
			}
		}while(pk_total_size);
		
		if(packet){
			//传递给应用
			do_callback(c,packet,0);
			if(packet != pk_buf)
				free(packet);
			packet = NULL;
		}
		
		if(c->status == lua_socket_close)
			return 0;
	}while(1);

	return 1;
}

static void luasocket_post_recv(lua_socket_t l){
	/*if(kn_socket_get_type(l->sock) == STREAM_SOCKET){
		l->recv_buf = new_bytebuffer(65536);
		l->wrecvbuf[0].iov_base = l->recvbuf->data;
		l->wrecvbuf[0].iov_len = 65536;
		l->recv_overlap.iovec_count = 1;
		l->recv_overlap.iovec = l->wrecvbuf;
		kn_post_recv(l->sock,&l->recv_overlap);
	}*/
}

static void luasocket_post_send(lua_socket_t l){
	if(kn_socket_get_type(l->sock) == STREAM_SOCKET){
		int c = 0;
		sendbuf *buf = (sendbuf*)kn_list_head(&l->send_list);
		while(c < 512 && buf){
			l->wsendbuf[c].iov_base = buf->buf + buf->index;//buf->buf+buf->index;
			l->wsendbuf[c].iov_len  = buf->size;
			++c;
			buf = (sendbuf*)buf->node.next;
		}
		l->send_overlap.iovec_count = c;
		l->send_overlap.iovec = l->wsendbuf;
		kn_post_send(l->sock,&l->send_overlap);		
	}
}

static void stream_recv_finish(lua_socket_t l,st_io *io,int32_t bytestransfer,int32_t err)
{
	if(bytestransfer <= 0){
		do_callback(l,NULL,err);
	}else{
		update_recv_pos(l,bytestransfer);
		l->unpack_size += bytestransfer;
		if(!unpack(l)) return;
		luasocket_post_recv(l);
	}
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
	//printf("stream_transfer_finish\n");
    lua_socket_t l = (lua_socket_t)kn_socket_getud(s);
    kn_ref_acquire(&l->ref);
    //防止l在callback中被释放
    if(!io){
		lua_rawgeti(l->callbackObj->L,LUA_REGISTRYINDEX,l->callbackObj->rindex);
		lua_pushstring(l->callbackObj->L,"recvfinish");
		lua_gettable(l->callbackObj->L,-2);
		if(l->callbackObj->L != g_L) lua_xmove(l->callbackObj->L,g_L,1);
		lua_pushlightuserdata(g_L,l);
		lua_pushnil(g_L);
		lua_pushnumber(g_L,err);				
		if(0 != lua_pcall(g_L,3,0,0))
		{
			const char * error = lua_tostring(g_L, -1);
			printf("stream_transfer_finish:%s\n",error);
			lua_pop(g_L,1);
		}
		lua_pop(g_L,1);                 		
	}else if(io == &l->send_overlap)
		stream_send_finish(l,io,bytestransfer,err);
	else if(io == &l->recv_overlap)
		stream_recv_finish(l,io,bytestransfer,err);
	kn_ref_release(&l->ref);
}


static void on_accept(kn_socket_t s,void *ud){
	printf("c on_accept\n");
	lua_socket_t c = new_luasocket(s,NULL);
	luaObject_t  callbackObj = (luaObject_t)ud;
	kn_socket_setud(s,c);
	kn_ref_acquire(&c->ref);
	lua_rawgeti(callbackObj->L,LUA_REGISTRYINDEX,callbackObj->rindex);
	lua_pushstring(callbackObj->L,"onaccept");
	lua_gettable(callbackObj->L,-2);
	if(callbackObj->L != g_L) lua_xmove(callbackObj->L,g_L,1);
	lua_pushlightuserdata(g_L,c);			
	if(0 != lua_pcall(g_L,1,0,0))
	{
		const char * error = lua_tostring(g_L, -1);
		printf("on_accept:%s\n",error);
		lua_pop(g_L,1);		
	}
	lua_pop(g_L,1);
	kn_ref_release(&c->ref);  		
}


static int lua_closefd(lua_State *L){
	lua_socket_t l = lua_touserdata(L,1);
	luasocket_close(l);
	return 0;
}


static void on_connect(kn_socket_t s,struct kn_sockaddr *remote,void *ud,int err)
{	
	luaObject_t obj = (luaObject_t)ud;	
	lua_socket_t l = NULL;
	if(s){
		l = new_luasocket(s,NULL);
		kn_socket_setud(s,l);
	}
	lua_rawgeti(obj->L,LUA_REGISTRYINDEX,obj->rindex);
	lua_pushstring(obj->L,"onconnected");
	lua_gettable(obj->L,-2);
	if(obj->L != g_L) lua_xmove(obj->L,g_L,1);
	if(l) lua_pushlightuserdata(g_L,l);
	else  lua_pushnil(g_L);
	
	lua_newtable(g_L);
	lua_pushstring(g_L,"type");
	lua_pushnumber(g_L,remote->addrtype);
	lua_settable(g_L, -3);
	if(remote->addrtype == AF_INET){
		char ip[32];
		inet_ntop(AF_INET,&remote->in,ip,(socklen_t )sizeof(remote->in));
		lua_pushstring(g_L,"ip");
		lua_pushstring(g_L,ip);		
		lua_settable(g_L, -3);
		lua_pushstring(g_L,"port");
		lua_pushnumber(g_L,ntohl(remote->in.sin_port));		
		lua_settable(g_L, -3);			
	}
	lua_pushnumber(g_L,err);
	
	kn_ref_acquire(&l->ref);
	
	if(0 != lua_pcall(g_L,3,0,0))
	{
		const char * error = lua_tostring(g_L, -1);
		printf("on_connect:%s\n",error);
		lua_pop(g_L,1);		
	}
	lua_pop(g_L,1);
	kn_ref_release(&l->ref);
	release_luaObj(obj);
}

int lua_connect(lua_State *L){
	
	int proto = lua_tonumber(L,1);
	int sock_type = lua_tonumber(L,2);
	luaObject_t remote = create_luaObj(L,3);
	luaObject_t local = create_luaObj(L,4);
	luaObject_t obj = create_luaObj(L,5);
	int timeout = lua_tonumber(L,6);
	if(!remote || !obj){
		release_luaObj(local);
		release_luaObj(remote);
		release_luaObj(obj);
		lua_pushboolean(L,0);
		return 1;
	}
	kn_sockaddr addr_local;
	kn_sockaddr addr_remote;
	if(remote){
		int type = GET_OBJ_FIELD(remote,"type",int,lua_tonumber);
		if(type == AF_INET){
			kn_addr_init_in(&addr_remote,
						    GET_OBJ_FIELD(remote,"ip",const char*,lua_tostring),
						    GET_OBJ_FIELD(remote,"port",int,lua_tonumber));	
		}
	}
	if(local){
		int type = GET_OBJ_FIELD(local,"type",int,lua_tonumber);
		if(type == AF_INET){
			kn_addr_init_in(&addr_local,
						    GET_OBJ_FIELD(local,"ip",const char*,lua_tostring),
						    GET_OBJ_FIELD(local,"port",int,lua_tonumber));	
		}	
	}
			
	if(0 != kn_asyn_connect(g_proactor,proto,sock_type,local?&addr_local:NULL,
					&addr_remote,on_connect,(void*)obj,(int64_t)timeout))
	{
		release_luaObj(obj);
		lua_pushboolean(L,0);
	}else
		lua_pushboolean(L,1);
		
	release_luaObj(local);
	release_luaObj(remote);	
	return 1;
}


int lua_listen(lua_State *L){
	int proto = lua_tonumber(L,1);
	int sock_type = lua_tonumber(L,2);
	luaObject_t addr = create_luaObj(L,3);
	kn_sockaddr addr_local;
	luaObject_t callbackObj;
	int type = GET_OBJ_FIELD(addr,"type",int,lua_tonumber);
	if(type == AF_INET){
		kn_addr_init_in(&addr_local,
						GET_OBJ_FIELD(addr,"ip",const char*,lua_tostring),
						GET_OBJ_FIELD(addr,"port",int,lua_tonumber));
		callbackObj =  create_luaObj(L,4);			 
		lua_socket_t l = new_luasocket(kn_listen(g_proactor,proto,sock_type,&addr_local,on_accept,(void*)callbackObj),
									   callbackObj);
		lua_pushlightuserdata(L,l);
		release_luaObj(addr);
		return 1;			
	}
	release_luaObj(addr);
	lua_pushnil(L);
	return 1;
}

int lua_send(lua_State *L){
	lua_socket_t l  = lua_touserdata(L,1);
	sendbuf *buf;
	const char* str = NULL;
	
	if(lua_isstring(L,2)){
		str = lua_tostring(L,2);
		int size = strlen(str)+1;
		buf = calloc(1,sizeof(*buf)+size);
		strcpy(buf->buf,str); 
	}else
	{
		lua_pushboolean(L,0);	
		return 1;
	}
	luaObject_t  addr_remote = NULL;
	if(!lua_isnil(L,3))
		addr_remote = create_luaObj(L,3);
	
	release_luaObj(addr_remote);
	//if(addr_remote)
	//	buf->remote = *addr_remote;
	kn_list_pushback(&l->send_list,(kn_list_node*)buf);
	
	luasocket_post_send(l);
	lua_pushboolean(L,1);	
	return 1;
}

static void p_run(){
 	uint64_t tick,now;
    tick = now = kn_systemms64();	
	while(!recv_sigint){
		kn_proactor_run(g_proactor,50);
        now = kn_systemms64();
		if(now - tick > 1000)
		{
            uint32_t elapse = (uint32_t)(now-tick);
            printf("count:%d/s\n",recv_count);
			tick = now;
			recv_count = 0;
		}			
	}	
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

/*int lua_release_bytebuffer(lua_State *L){
	bytebuffer_t b = (bytebuffer_t)lua_touserdata(L,1);
	kn_ref_release(&b->ref); 
	return 0; 
}*/

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
    
	lua_register(L,"listen",&lua_listen);
	lua_register(L,"_connect",&lua_connect); 
    lua_register(L,"_send",&lua_send);
	lua_register(L,"close",&lua_closefd);
	lua_register(L,"_bind",&lua_bind); 
	//lua_register(L,"release_bytebuffer",&lua_release_bytebuffer); 
	 
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
	p_run();	
	return 0;
} 

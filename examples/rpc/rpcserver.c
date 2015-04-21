//c service for rpc test

#include "kendynet.h"
#include "connection.h"
#include "kn_timer.h"
#include "lua/lua_util.h"
#include "lua/lua_util_packet.h"


int  client_count = 0;
int  rpc_count = 0;

static luaRef_t luaRPCHandle;

struct rpc_record{
	uint64_t h;
	uint32_t l;
};

const char *str_lua_process_rpc_call = "\
	local function process_rpc_call(rpk,c)\
		local request = C.rpk_read_table(rpk)\
		if not request then\
			return\
		end\
		local identity = request.identity\
		local error,func = C.check_and_get_function(c,request.f,identity.h,identity.l)\
		if not error and not func then\
			return\
		elseif func then\
			C.rpc_response(c,{identity = request.identity,ret={func(table.unpack(request.arg))}})\
		else\
			C.rpc_response(c,{identity = request.identity,err=error})\
		end\
	end\
	return process_rpc_call" ;


static int rpc_Plus(lua_State *L){
	int a = lua_tointeger(L,1);
	int b = lua_tointeger(L,2);
	lua_pushinteger(L,a+b);         //result
	++rpc_count;
	return 1;
}

static int lua_check_and_get_function(lua_State *L){
	connection_t c = lua_touserdata(L,1);
	const char *function_name = lua_tostring(L,2);
	struct rpc_record *r = (struct rpc_record*)connection_getud(c);
	if(!r){
		r = calloc(1,sizeof(*r));
		r->h = 0;
		r->l = 0;
		connection_setud(c,r,NULL);
	}
	uint64_t h = lua_tointeger(L,3);
	uint32_t l = lua_tointeger(L,4);
	if(h > r->h || l > r->l){
		r->h = h;
		r->l = l;
		if(strcmp(function_name,"Plus") == 0){
			lua_pushnil(L);
			lua_pushcfunction(L,rpc_Plus);
			return 2;
		}else{
			lua_pushstring(L,"function not found");
			return 1;
		}
	}
	return 0;
}

static int lua_rpc_response(lua_State *L){
	connection_t c = lua_touserdata(L,1);
	wpacket_t wpk = wpk_create(512);
	wpk_write_uint32(wpk,0xDBCAABCD);
	const char *errmsg = lua_pack_table(wpk,L,2);
	if(errmsg)
		luaL_error(L,errmsg);
	connection_send(c,(packet_t)wpk,NULL);
	return 0;
}

static int lua_rpk_read_table(lua_State *L){
	rpacket_t rpk = lua_touserdata(L,1);
	lua_unpack_table(rpk,L);
	return 1;
}

void  on_packet(connection_t c,packet_t p){
	rpacket_t rpk = (rpacket_t)p;
	int cmd = rpk_read_uint32(rpk);
	if(cmd == 0xABCDDBCA){
		const char *err = LuaCallRefFunc(luaRPCHandle,"pp",rpk,c);
		if(err){
			printf("%s\n",err);
		}
	}
}

void on_disconnected(connection_t c,int err){
	void *r = connection_getud(c);
	if(r) free(r);
	printf("on_disconnectd\n");
	--client_count;
}

void on_accept(handle_t s,void *listener,int _2,int _3){
	engine_t p = kn_sock_engine((handle_t)listener);
	connection_t conn = new_connection(s,4096,new_rpk_decoder(4096));
	connection_associate(p,conn,on_packet,on_disconnected);
	++client_count;
	printf("%d\n",client_count);   
}


int timer_callback(kn_timer_t timer){
	printf("client_count:%d,rpc_count:%d\n",client_count,rpc_count);
	rpc_count = 0;
	return 1;
}

int toname_init(engine_t engine,const char *ip,uint16_t port);
int toname_register_service(const char *func,const char *ip,uint16_t port);

int main(int argc,char **argv){
	signal(SIGPIPE,SIG_IGN);
	engine_t p = kn_new_engine();

	if(0 != toname_init(p,"127.0.0.1",8080)){
		printf("toname_init failed\n");
		exit(0);
	}

	if(0 != toname_register_service("Plus",argv[1],atoi(argv[2]))){
		printf("register Plus failed\n");
		exit(0);
	}

	kn_sockaddr local;
	kn_addr_init_in(&local,argv[1],atoi(argv[2]));
	handle_t l = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(0 == kn_sock_listen(l,&local)){
		kn_engine_associate(p,l,on_accept);
	}else{
		printf("listen error\n");
		return 0;
	}
	kn_reg_timer(p,1000,timer_callback,NULL);

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
    	luaL_Reg lib[] = {
        		{"rpk_read_table",lua_rpk_read_table},
        		{"check_and_get_function",lua_check_and_get_function},
        		{"rpc_response",lua_rpc_response},       		              		                   
        		{NULL, NULL}
    	};
    	luaL_newlib(L, lib);
	lua_setglobal(L,"C");	
	luaRPCHandle = makeLuaObjByStr(L,str_lua_process_rpc_call);
	kn_engine_run(p);
	return 0;
}
#include "kendynet.h"
#include "connection.h"
#include "lua/lua_util.h"
#include "lua/lua_util_packet.h"
#include "kn_list.h"
#include "kn_timer.h"

static engine_t g_engine = NULL;
static lua_State *L = NULL;
static connection_t con_toname = NULL;
static int connect_err = 0;
//static int register_response;
static kn_list all_services;
static luaRef_t luaObj;
static kn_timer_t timer;

enum{
	REG_SUCCESS = 1,
	REG_FAILED = 2,
	REG_DUP = 3,
};

char toname_ip[32];
uint16_t toname_port;

struct stService{
	kn_list_node node;
	char        name[256];
	char        ip[32];
	uint16_t port;
};

struct stRPC_context{
	uint64_t     identityh;
	uint32_t     identityl;
	uint32_t     ret;
	uint8_t       trycount;
	packet_t    wpk;
	timer_t      timer;
}g_context;


static int lua_rpk_read_table(lua_State *L){
	rpacket_t rpk = lua_touserdata(L,1);
	lua_unpack_table(rpk,L);
	return 1;
}

static int lua_wpk_write_table(lua_State *L){
	wpacket_t wpk = lua_touserdata(L,1);
	const char *errmsg = lua_pack_table(wpk,L,2);
	if(errmsg)
		luaL_error(L,errmsg);	
	return 0;
}

static int timer_rpccontext_callback(kn_timer_t timer){
	++g_context.trycount;
	if(!con_toname || g_context.trycount > 3){
		g_context.ret = REG_FAILED;
	}else{
		packet_t wpk = make_writepacket(g_context.wpk);
		//resend
		if(0 != connection_send(con_toname,(packet_t)wpk,NULL)){
			g_context.ret = REG_FAILED;
		}
	}
	if(g_context.ret == REG_FAILED){
		destroy_packet(g_context.wpk);
		g_context.wpk = NULL;
		g_context.timer = NULL;
		return 0;		
	}
	return 1;
}

static int _register_service(const char *func,const char *ip,uint16_t port){
	if(!con_toname) return -1;
	wpacket_t wpk = wpk_create(512);
	wpk_write_uint32(wpk,0xABCDDBCA);
	const char *err = LuaCallTabFunc(luaObj,"pack_register_request","pssi:ii",wpk,func,ip,port,&g_context.identityh,&g_context.identityl);
	if(!err){
		if(0 == connection_send(con_toname,(packet_t)wpk,NULL)){
			g_context.wpk = make_writepacket((packet_t)wpk);
			g_context.trycount = 1;
			g_context.ret = 0;
			g_context.timer = kn_reg_timer(g_engine,5000,timer_rpccontext_callback,NULL);
			while(con_toname && !g_context.ret){
				kn_engine_runonce(g_engine,5000,10000);
			}
			if(g_context.timer){
				kn_del_timer(g_context.timer);
				g_context.timer = NULL;
			}
			if(g_context.wpk){
				destroy_packet(g_context.wpk);
				g_context.wpk = NULL;
			}
			return g_context.ret ==  REG_SUCCESS ? 0:-1;
		}
		return -1;
	}
	destroy_packet(wpk);	
	return -1;	
}

static int timer_callback(kn_timer_t timer){
	wpacket_t wpk = wpk_create(512);
	wpk_write_uint32(wpk,0xABABCBCB);
	connection_send(con_toname,(packet_t)wpk,NULL);	
	return 1;
}

static void on_packet(connection_t c,packet_t p){
	rpacket_t rpk = (rpacket_t)p;
	int cmd = rpk_read_uint32(rpk);
	if(cmd == 0xDBCAABCD){
		int ret;
		const char *err = LuaCallTabFunc(luaObj,"fetch_response","pii:i",rpk,g_context.identityh,g_context.identityl,&ret);
		if(err){
			//log
			printf("%s\n",err);
			g_context.ret = REG_FAILED;
		}else{
			if(ret != 3)
				g_context.ret = ret;
		}		
	}	
}

static void on_disconnected(connection_t c,int err){
	con_toname = NULL;
	kn_del_timer(timer);
	timer = NULL;
}

static void on_connect(handle_t s,void *_1,int _2,int err){
	if(err){
		connect_err = err;
	}else{
		con_toname = new_connection(s,4096,new_rpk_decoder(4096));
		connection_associate(g_engine,con_toname,on_packet,on_disconnected);
		int size = kn_list_size(&all_services);
		int i = 0;
		struct stService *st = (struct stService*)kn_list_head(&all_services);
		for(; i < size; ++i){
			if(0 != _register_service(st->name,st->ip,st->port)){
				//log
				printf("register %s failed\n",st->name);
			}
			st = (struct stService*)((kn_list_node*)st)->next;
		}
		timer = kn_reg_timer(g_engine,5000,timer_callback,NULL);				
	}
}


static int _connect(){
	handle_t c = kn_new_sock(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	kn_sockaddr remote;
	kn_addr_init_in(&remote,toname_ip,toname_port);	
	int ret = kn_sock_connect(c,&remote,NULL);
	if(ret > 0){
		on_connect(c,NULL,0,0);
	}else if(ret == 0){
		kn_engine_associate(g_engine,c,on_connect);
		connect_err = 0;
	}else{
		kn_close_sock(c);
	}	
	return ret;
}

static const char *luaStr = "\
	local t = {}\
	local REG_SUCCESS = 1\
	local REG_FAILED = 2\
	local REG_DUP = 3\
	local function gen_rpc_identity()\
		local g_counter = 0\
		return function ()\
			g_counter = math.modf(g_counter + 1,0xffffffff)\
			return {h=os.time(),l=g_counter} \
		end	\
	end	\
	gen_rpc_identity = gen_rpc_identity()\
	t.pack_register_request = function (wpk,name,ip,port)\
		local request = {}\
		request.f = [[Register]]\
		request.identity = gen_rpc_identity()\
		request.arg = {name,ip,port}\
		C.wpk_write_table(wpk,request)\
		return request.identity.h,request.identity.l\
	end\
	t.fetch_response = function (rpk,identityh,identityl)\
		local response = C.rpk_read_table(rpk)\
		if response.identity.h ~= identityh or response.identity.l ~= identityl then\
			return REG_DUP\
		end\
		if response.err or not response.ret or not response.ret[1] then\
			return REG_FAILED\
		end\
		return REG_SUCCESS\
	end\
	return t";

int toname_init(engine_t engine,const char *ip,uint16_t port){
	if(engine && ip){
		strncpy(toname_ip,ip,32);
		toname_port = port;
		g_engine = engine;
		kn_list_init(&all_services);

		L = luaL_newstate();
		luaL_openlibs(L);
	    	luaL_Reg lib[] = {
	        		{"rpk_read_table",lua_rpk_read_table},
	        		{"wpk_write_table",lua_wpk_write_table}, 		              		                   
	        		{NULL, NULL}
	    	};
	    	luaL_newlib(L, lib);
		lua_setglobal(L,"C");

		luaObj = makeLuaObjByStr(L,luaStr);
		if(!luaObj.L){
			lua_close(L);
			L = NULL;
			return -1;
		}

		int ret = _connect();
		if(con_toname)
			return 0;
		else if(ret == 0){
			while(!con_toname && !connect_err){
				kn_engine_runonce(g_engine,5000,10000);
			}
			return con_toname? 0:-1;			
		}
	}
	return -1;
}

int toname_register_service(const char *func,const char *ip,uint16_t port){
	if(0 == _register_service(func,ip,port)){
		struct stService *st = (struct stService*)calloc(1,sizeof(*st));
		strncpy(st->name,func,256);
		strncpy(st->ip,ip,32);
		st->port = port;
		kn_list_pushback(&all_services,(kn_list_node*)st);		
		return 0;
	}
	printf("register %s failed\n",func);
	return -1;
}


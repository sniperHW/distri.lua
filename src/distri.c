#include "kendynet.h"
#include "lua_util.h"
#include "luasocket.h"
#include "luaredis.h"
#include "lualog.h"
#include "kn_timer.h"
#include "kn_time.h"
#include "debug.h"


__thread engine_t g_engine = NULL;
__thread int      g_status = 1;

static void sig_int(int sig){
	g_status = 0;
	kn_stop_engine(g_engine);
}

static int  lua_stop(lua_State *L){
	kn_stop_engine(g_engine);
	return 0;
}

static int  lua_getsystick(lua_State *L){
	lua_pushnumber(L,kn_systemms());
	return 1;
}

/*static void set_luaarg(lua_State *L,int argc,char **argv)
{
	if(argc > 2){
		int i = 2;
		int j = 1;
		lua_newtable(L);		
		for(; i < argc; ++i,++j){
			lua_pushstring(L,argv[i]);
			lua_rawseti(L,-2,j);
		}		
		lua_setglobal(L,"args");			
	}	
}*/

static void start(lua_State *L,int argc,char **argv)
{
	const char *start_file = argv[1];
	if (luaL_dofile(L,"lua/distri.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
		exit(0);
	}
	const char *error = LuaCall(L,"distri_lua_start_run","s",start_file);
	if(error){
		printf("%s\n",error);
		exit(0);
	}
/*	printf("start\n");
	char buf[4096];
	snprintf(buf,4096,"\
		local Sche = require \"lua/sche\"\
		local Http = require \"lua/http\"\
		local main,err= loadfile(\"%s\")\
		if err then print(err) end\
		Sche.Spawn(function () main() end)\
		local ms = 5\
		while RunOnce(ms) do\
			if Sche.Schedule() > 0 then\
				ms = 0\
			else\
				ms = 5\
			end\
		end",start_file);		
	set_luaarg(L,argc,argv);	
	int oldtop = lua_gettop(L);
	luaL_loadstring(L,buf);
	const char *error = luacall(L,NULL);
	if(error){
		lua_settop(L,oldtop);
		printf("%s\n",error);
		return;
	}
	lua_settop(L,oldtop);*/

}


static int lua_RunOnce(lua_State *L){
	if(g_status){ 
		uint32_t ms = (uint32_t)lua_tointeger(L,1);
		kn_engine_runonce(g_engine,ms);
	}
	lua_pushboolean(L,g_status);
	return 1;
}

void reg_luahttp(lua_State *L);
int main(int argc,char **argv)
{
	if(argc < 2){
		printf("usage distrilua luafile\n");
		return 0;
	}
	
	/*if(debug_init()){
		printf("debug_init failed\n");
		return 0;
	}*/
	
	lua_State *L = luaL_newstate();	
	luaL_openlibs(L);
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
	lua_register(L,"stop_program",lua_stop); 
	lua_register(L,"GetSysTick",lua_getsystick); 
	lua_register(L,"RunOnce",lua_RunOnce); 	   	
	reg_luasocket(L);
	reg_luahttp(L);
	reg_luaredis(L);
	reg_lualog(L);
	g_engine = kn_new_engine();
	start(L,argc,argv);
	return 0;
} 

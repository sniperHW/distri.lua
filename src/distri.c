#include "kendynet.h"
#include "lua_util.h"
#include "luasocket.h"
#include "luaredis.h"
#include "lualog.h"
#include "kn_timer.h"
#include "kn_time.h"
#include "debug.h"
#include "myprocps/mytop.h"
#include "kn_daemonize.h"


engine_t      g_engine = NULL;
volatile int  g_status = 1;

static void sig_int(int sig){
	printf("sig_int\n");
	g_status = 0;
	kn_stop_engine(g_engine);
}

static int  lua_getsystick(lua_State *L){
	lua_pushnumber(L,kn_systemms());
	return 1;
}

static void set_luaarg(lua_State *L,int argc,char **argv)
{
	if(argc){
		int i = 0;
		int j = 1;
		lua_newtable(L);		
		for(; i < argc; ++i,++j){
			lua_pushstring(L,argv[i]);
			lua_rawseti(L,-2,j);
		}		
		lua_setglobal(L,"arguments");			
	}	
}

static void start(lua_State *L,int argc,char **argv)
{
	const char *start_file = argv[1];
	if (luaL_dofile(L,"lua/distri.lua")) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
		exit(0);
	}
	set_luaarg(L,argc,argv);
	const char *error = LuaCall(L,"distri_lua_start_run","s",start_file);
	if(error){
		printf("%s\n",error);
		exit(0);
	}
}


//fork a process and exec a new image immediate
static int lua_ForkExec(lua_State *L){
	int top = lua_gettop(L);
	int ret = -1;
	char **argv = NULL;	
	if(top > 0){
		const char *exepath = lua_tostring(L,1);
		int size = top;
		if(size){
			argv = calloc(sizeof(*argv),size+1);
			argv[0] = (char*)exepath;
			int i = 2;
			for(;i <= top; ++i){
				argv[i-1] = (char*)lua_tostring(L,i);
			}
			argv[size] = NULL;
		}
		pid_t pid;
		if((pid = fork()) == 0 ){
			if(execv(exepath,argv) < 0)
				exit(-1);
		}else if(pid > 0)
			ret = 0;
	}
	if(argv) free(argv);	
	lua_pushinteger(L,ret);
	return 1;
}

static int lua_KillProcess(lua_State *L){
	pid_t pid = (pid_t)lua_tointeger(L,1);
	lua_pushinteger(L,kill(pid,SIGKILL));
	return 1;
}

static int lua_StopProcess(lua_State *L){
	pid_t pid = (pid_t)lua_tointeger(L,1);
	lua_pushinteger(L,kill(pid,SIGINT));
	return 1;
}

static int lua_RunOnce(lua_State *L){
	if(g_status){ 
		uint32_t ms = (uint32_t)lua_tointeger(L,1);
		kn_engine_runonce(g_engine,ms);
	}
	lua_pushboolean(L,g_status);
	return 1;
}

static int lua_Break(lua_State *L){
	return 0;
}

static int lua_Top(lua_State *L){
	lua_pushstring(L,top());
	return 1;
}

static int lua_AddTopFilter(lua_State *L){
	const char *str = lua_tostring(L,1);
	addfilter(str);
	return 0;
}

void reg_luahttp(lua_State *L);
 void reg_luabase64(lua_State *L);
int main(int argc,char **argv)
{
	if(argc < 2){
		printf("usage distrilua luafile\n");
		return 0;
	}
	int i = 1;
	for(; i < argc; ++i){
		if(strcasecmp("-d",argv[i]) == 0){
			daemonize();
			break;
		}
	}
	/*if(debug_init()){
		printf("debug_init failed\n");
		return 0;
	}*/	
	lua_State *L = luaL_newstate();	
	luaL_openlibs(L);
    	signal(SIGINT,sig_int);
    	signal(SIGPIPE,SIG_IGN);
    	luaL_Reg l[] = {
        		{"Break",lua_Break},
        		{"GetSysTick",lua_getsystick},
        		{"RunOnce",lua_RunOnce}, 
        		{"Top",lua_Top},
        		{"AddTopFilter",lua_AddTopFilter},
        		{"ForkExec",lua_ForkExec},
        		{"KillProcess",lua_KillProcess},
        		{"StopProcess",lua_StopProcess},          		              		                   
        		{NULL, NULL}
    	};
    	luaL_newlib(L, l);
	lua_setglobal(L,"C");    		
	reg_luasocket(L);
	reg_luahttp(L);
	reg_luaredis(L);
	reg_lualog(L);
	reg_luabase64(L);
	g_engine = kn_new_engine();
	start(L,argc,argv);
	return 0;
} 

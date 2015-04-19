#include "kendynet.h"
#include "lua/lua_util.h"
#include "luasocket.h"
#include "lualog.h"
#include "kn_timer.h"
#include "kn_time.h"
#ifdef _LINUX
#include "debug.h"
//#include "myprocps/mytop.h"
#endif
#include "kn_daemonize.h"
#include "wpacket.h"
#include "rpacket.h"
#include "rawpacket.h"
#include "kn_objpool.h"
#include "listener.h"
#include "top.h"

engine_t       g_engine = NULL;
struct top    *g_top = NULL;
volatile int  g_status = 1;

static void sig_int(int sig){
	g_status = 0;
	kn_stop_engine(g_engine);
}

static int  lua_getsystick(lua_State *L){
	lua_pushinteger(L,kn_systemms64());
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
		SYS_LOG(LOG_ERROR,"do distri.lua %s\n",error);
		exit(0);
	}
	set_luaarg(L,argc,argv);
	const char *error = LuaCall(L,"distri_lua_start_run","s",start_file);
	listener_stop();	
	if(error){
		SYS_LOG(LOG_ERROR,"distri_lua_start_run: %s\n",error);
		exit(0);
	}
}


/*fork a process and exec a new image immediate
    the new process will run as daemon
*/
static int lua_ForkExec(lua_State *L){
	int top = lua_gettop(L);
	int ret = -1;
	char **argv = NULL;	
	if(top > 0){
		int size = top;
		if(size){
			argv = calloc(sizeof(*argv),size+1);
			int i;
			for(i = 1;i <= top; ++i){
				if(!lua_isstring(L,i)) return luaL_error(L,"param should be string");
				argv[i-1] = (char*)lua_tostring(L,i);
			}
			argv[size] = NULL;
		}
		char *exepath = argv[0];
		pid_t pid;
		if((pid = fork()) == 0 ){	
			int i, fd0, fd1, fd2;
			struct rlimit	rl;
			struct sigaction	sa;
			umask(0);
			/*
			 * Get maximum number of file descriptors.
			 */
			if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
				exit(0);			
			setsid();
			/*
			 * Ensure future opens won't allocate controlling TTYs.
			 */
			sa.sa_handler = SIG_IGN;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = 0;
			if (sigaction(SIGHUP, &sa, NULL) < 0)
				exit(0);//err_quit("%s: can't ignore SIGHUP");
			if ((pid = fork()) < 0)
				exit(0);//err_quit("%s: can't fork", cmd);
			else if (pid != 0) /* parent */
				exit(0);		
			/*
			 * Close all open file descriptors.
			 */
			if (rl.rlim_max == RLIM_INFINITY)
				rl.rlim_max = 1024;
			for (i = 0; i < rl.rlim_max; i++)
				close(i);
			/*
			 * Attach file descriptors 0, 1, and 2 to /dev/null.
			 */
			//fd0 = open("./error.txt",O_CREAT|O_RDWR);
			fd0 = open("/dev/null", O_RDWR);
			fd1 = dup(0);
			fd2 = dup(0);
			/*
			 * Initialize the log file.
			 */
			//openlog(cmd, LOG_CONS, LOG_DAEMON);
			if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
				//syslog(LOG_ERR, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
				//char buf[4096];
				//snprintf(buf,4096,"unexpected file descriptors %d %d %d",fd0, fd1, fd2);
				//write(fd0,buf,strlen(buf));
				exit(1);
			}	
			if(execv(exepath,argv) < 0){
				exit(-1);
			}
		}else if(pid > 0)
			ret = 0;
	}
	if(argv) free(argv);	
	lua_pushinteger(L,ret);
	wait(NULL);
	return 1;
}

static int lua_KillProcess(lua_State *L){
	if(lua_gettop(L) < 1 || !lua_isnumber(L,1))
		return luaL_error(L,"need a integer param");
	pid_t pid = (pid_t)lua_tointeger(L,1);
	lua_pushinteger(L,kill(pid,SIGKILL));
	return 1;
}

static int lua_StopProcess(lua_State *L){
	if(lua_gettop(L) < 1 || !lua_isnumber(L,1))
		return luaL_error(L,"need a integer param");	
	pid_t pid = (pid_t)lua_tointeger(L,1);
	lua_pushinteger(L,kill(pid,SIGINT));
	return 1;
}

static int lua_RunOnce(lua_State *L){
	if(lua_gettop(L) < 1 || !lua_isnumber(L,1))
		return luaL_error(L,"need a integer param");		
	if(g_status){ 
		uint32_t ms = (uint32_t)lua_tointeger(L,1);
		uint32_t max_process_time = (uint32_t)lua_tointeger(L,2);
		kn_engine_runonce(g_engine,ms,max_process_time);
	}
	lua_pushboolean(L,g_status);
	return 1;
}

static int lua_Break(lua_State *L){
	return 0;
}

static int lua_Top(lua_State *L){
	if(!g_top) g_top = new_top();
	kn_string_t str = get_top(g_top);
	if(str){
		lua_pushstring(L,kn_to_cstr(str));
		kn_release_string(str);
		return 1;
	}
	return 0;
}

static int lua_AddTopFilter(lua_State *L){
	if(!g_top) g_top = new_top();
	if(lua_gettop(L) < 1)
		return luaL_error(L,"need a least one param");		
	int i;
	int size = lua_gettop(L);
	for(i = 1; i <= size; ++i){
		if(!lua_isstring(L,i)) 
			return luaL_error(L,"param should be string");
		add_filter(g_top,lua_tostring(L,i));
	}		
	return 0;
}

static int lua_GetPid(lua_State *L){
	lua_pushinteger(L,getpid());
	return 1;
}

void reg_luahttp(lua_State *L);
void reg_luabase64(lua_State *L);
void reg_timeutil(lua_State *L);
void reg_luaredis(lua_State *L);
void reg_luaminheap(lua_State *L);
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
	g_wpk_allocator = objpool_new(sizeof(struct wpacket),100000);
	g_rpk_allocator = objpool_new(sizeof(struct rpacket),100000);
	g_rawpk_allocator = objpool_new(sizeof(struct rawpacket),100000);
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
        		{"GetPid",lua_GetPid},         		              		                   
        		{NULL, NULL}
    	};
    	luaL_newlib(L, l);
	lua_setglobal(L,"C");
	g_engine = kn_new_engine();	    		
	reg_luasocket(L);
	reg_luahttp(L);
	reg_luaredis(L);
	reg_lualog(L);
	reg_luabase64(L);
	reg_timeutil(L);
	reg_luaminheap(L);
	start(L,argc,argv);
	return 0;
} 

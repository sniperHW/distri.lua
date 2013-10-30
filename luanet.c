/*	
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/	
#include "lua.h"  
#include "lauxlib.h"  
#include "lualib.h"  
#include "util/link_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "netservice.h"

enum
{
	LNUMBER = 1,
	LSTRING,
};

enum
{
	CNUMBER = 1,
	CPACKET = 2,
};

static uint16_t recv_sigint = 0;

struct luaNetService
{
	int m_iKeyIndex;
	lua_State *L;
	void (*call)(lua_State *L,
				 int index,
				 const char *name,
				 struct connection *c,
				 rpacket_t rpk);
	struct netservice  *net;
};


static inline void push_rpacket(lua_State *L,rpacket_t rpk)
{
	lua_newtable(L);
	int i;
	for(i = 0; rpk_data_remain(rpk) > 0; ++i)
	{
		uint8_t type = rpk_read_uint8(rpk);
		if(type == LNUMBER)
			lua_pushnumber(L,rpk_read_uint32(rpk));
		else if(type == LSTRING)
			lua_pushstring(L,rpk_read_string(rpk));
		else
			lua_pushnil(L);//不支持的类型
		lua_rawseti(L,-2,i+1);	
	}
}

static inline void push_msg(lua_State *L,struct connection *c,rpacket_t rpk)
{
	lua_newtable(L);
	if(c)
		lua_pushlightuserdata(L,c);
	else
		lua_pushnil(L);
	lua_rawseti(L,-2,1);
	if(rpk)
		push_rpacket(L,rpk)
	else
		lua_pushnil(L);
	lua_rawseti(L,-2,2);
}

static void callObjFunction(lua_state *L,
					 int index,
					 const char *name,
				     struct connection *c,
				     rpacket_t rpk)
{

	lua_rawgeti(L,LUA_REGISTRYINDEX,index);
	lua_pushstring(L,name);
	lua_gettable(L,-2);
	lua_rawgeti(L,LUA_REGISTRYINDEX,index);
	push_msg(L,c,rpk);
	if(lua_pcall(L,1,0,0) != 0)
	{
		const char *error = lua_tostring(L,-1);
		printf("%s\n",error);
		lua_pop(L,1);
	}
}


static int luaGetSysTick(lua_State *L){
	lua_pushnumber(L,GetSystemMs());
	return 1;
}

static void sig_int(int sig){
	recv_sigint = 1;
}

static int netservice_new(lua_State *L)
{
	struct netservice *service = new_service(100,100*3000);
	if(!service)
		lua_pushnil(L);
	else
	{
		struct luaNetService *lnet = calloc(1,sizeof(*lnet));
		lnet->m_iKeyIndex = luaL_ref(L,LUA_REGISTRYINDEX);
		lnet->net = service;
		lnet->call = callObjFunction;
		lua_pushlightuserdata(L,lnet);
	}
	return 1;
}

static int netservice_delete(lua_State *L)
{
	struct luaNetService *lnet = (struct luaNetService *)lua_touserdata(L,1);
	if(lnet){
		destroy_service(&lnet->net);
		if(lnet->m_iKeyIndex > 0){
			luaL_unref(L,LUA_REGISTRYINDEX,lnet->m_iKeyIndex);
		}
		free(lnet);
	}
	return 0;
}

static void on_process_packet(struct connection *c,rpacket_t rpk)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_data;	
	service->call(netobj->L,netobj->m_iKeyIndex,"process_packet",c,rpk);
}

static void c_recv_timeout(struct connection *c)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_data;
	service->call(netobj->L,netobj->m_iKeyIndex,c,"recv_timeout",NULL);
}

static void c_send_timeout(struct connection *c)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_data;
	service->call(netobj->L,netobj->m_iKeyIndex,c,"send_timeout",NULL);
}

static void on_disconnect(struct connection *c,uint32_t reason)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_data;
	service->call(netobj->L,netobj->m_iKeyIndex,c,"on_disconnect",NULL);
}

static void on_accept(SOCK s,void*ud)
{
	struct luaNetService *service = (struct luaNetService *)ud;
	struct connection *c = new_conn(s,0);
	c->usr_data = (uint64_t)service;
	service->net->bind(tcpclient,c
						,5000,c_recv_timeout,5000,c_send_timeout
						,on_process_packet,on_disconnect);
	service->call(service->L,service->m_iKeyIndex,"on_accept",c,NULL);
}

static void on_connect(SOCK s,void*ud,int err)
{
	if(s != INVALID_SOCK){
		struct luaNetService *service = (struct luaNetService *)ud;
		struct connection *c = new_conn(s,0);
		c->usr_data = (uint64_t)service;
		service->net->bind(service,c
							,5000,c_recv_timeout,5000,c_send_timeout
							,on_process_packet,remove_client);
		service->call(service->L,service->m_iKeyIndex,"on_connect",c,NULL);
	}
}

static int lua_active_close(lua_State *L)
{
	struct connection *c = (struct connection *)lua_touserdata(L,1);
	active_close(c);
}

void on_pkt_send_finish(struct connection *c,wpacket_t wpk)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_data;
	service->call(service->L,service->m_iKeyIndex,"on_send_finish",c,NULL);
}


wpacket_t luaGetluaWPacket(lua_State *L,int idx)
{
	int len = lua_objlen(L, idx);
	if(len % 2 != 0) return NULL;	
	int i;
	wpacket_t wpk = wpk_create(128,0);
	for(i = 1; i <= len; i+=2)
	{
		lua_rawgeti(L,-1,i);
		if(!lua_isnumber(L,-1))return NULL;
		wpk_write_uint8(wpk,(uint8_t)lua_tonumber(L,-1));//1:number,2:string
		lua_rawgeti(L,-1,i+1);
		if(lua_isnumber(L,-1))
			wpk_write_uint32(wpk,(uint32_t)lua_tonumber(L,-1));
		else if(lua_isstring(L,-1))
			wpk_write_string(wpk,lua_tostring(L,-1));
		else
		{
			wpk_destroy(&wpk);
			return NULL;
		}
	}
	return wpk;
}

static int luaSendPacket(lua_State *L)
{
	int arg_size = lua_gettop(L);
	struct connection *c = (struct connection *)lua_touserdata(L,1);
	wpacket_t w = luaGetluaWPacket(L,2);
	if(w){	
		lua_pushnumber(L,send_packet(c,w,arg_size == 3? on_pkt_send_finish:NULL));
	}
	else
		lua_pushnumber(L,-1);
	return 1;
}

static int luaConnect(lua_State *L)
{
	struct luaNetEngine *engine = (struct luaNetEngine *)lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	uint16_t port = (uint16_t)lua_tonumber(L,3);
	uint32_t timeout = lua_tonumber(L,4);
	engine->net->connect(engine->net,ip,port,(void*)engine,on_connect,timeout);
	return 0;
}

static int luaListen(lua_State *L)
{
	struct luaNetEngine *engine = (struct luaNetEngine *)lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	uint16_t port = (uint16_t)lua_tonumber(L,3);
	engine->net->listen(engine->net,ip,port,(void*)engine,on_accept);
	return 0;
}

static int luaEngineRun(lua_State *L)
{
	struct luaNetEngine *engine = (struct luaNetEngine *)lua_touserdata(L,1);
	uint32_t ms = lua_tonumber(L,2);
	if(recv_sigint)
		lua_pushnumber(L,-1);
	else
		lua_pushnumber(L,engine->net->loop(engine->net,ms));
	return 1;
}

void RegisterNet(lua_State *L){  
    
	lua_register(L,"Listen",&luaListen);
	lua_register(L,"Connect",&luaConnect);
	lua_register(L,"netservice_new",&netservice_new);   
    lua_register(L,"netservice_delete",&netservice_delete);
	lua_register(L,"active_close",&lua_active_close);   
    lua_register(L,"GetSysTick",&luaGetSysTick);
	lua_register(L,"EngineRun",&luaEngineRun);
	lua_register(L,"SendPacket",&luaSendPacket);  	
	InitNetSystem();
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
    printf("load c function finish\n");  
} 

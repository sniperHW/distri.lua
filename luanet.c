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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "core/netservice.h"


static uint16_t recv_sigint = 0;

struct luaNetService
{
	int m_iKeyIndex;
	lua_State *L;
	void (*call)(lua_State *L,
				 int index,
				 const char *name,
				 struct connection *c,
				 char *rpk);
	struct netservice  *net;
};


static void callObjFunction(lua_State *L,
					 int index,
					 const char *name,
				     struct connection *c,
				     char *rpk)
{
	lua_rawgeti(L,LUA_REGISTRYINDEX,index);
	lua_pushstring(L,name);
	lua_gettable(L,-2);
	if(lua_isnil(L,-1))
	{
		printf("%s is nil\n",name);
		return;
	}
	lua_rawgeti(L,LUA_REGISTRYINDEX,index);
	if(!c)return;
	lua_pushlightuserdata(L,c);
	int argc = 2;
	if(rpk){
		lua_pushstring(L,rpk);
		++argc;
	}
	if(lua_pcall(L,argc,0,0) != 0)
	{
		const char *error = lua_tostring(L,-1);
		printf("%s,%s\n",error,name);
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
	struct netservice *service = new_service();
	if(!service)
		lua_pushnil(L);
	else
	{
		struct luaNetService *lnet = calloc(1,sizeof(*lnet));
		lnet->m_iKeyIndex = luaL_ref(L,LUA_REGISTRYINDEX);
		lnet->net = service;
		lnet->call = callObjFunction;
		lnet->L = L;
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

static void lua_process_packet(struct connection *c,rpacket_t rpk)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_ptr;	
	uint32_t len = 0;
	const void *ptr = rpk_read_binary(rpk,&len);
	if(len < 4096)
	{
		char buf[4096];
		memcpy(buf,ptr,len);
		buf[len] = '\0';
		service->call(service->L,service->m_iKeyIndex,"process_packet",c,buf);
	}
	else
	{
		char *buf = calloc(len,sizeof(*buf));
		memcpy(buf,ptr,len);
		buf[len] = '\0';
		service->call(service->L,service->m_iKeyIndex,"process_packet",c,buf);
		free(buf);		
	}
}

static void lua_recv_timeout(struct connection *c)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_ptr;
	service->call(service->L,service->m_iKeyIndex,"recv_timeout",c,NULL);
}

static void lua_send_timeout(struct connection *c)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_ptr;
	service->call(service->L,service->m_iKeyIndex,"send_timeout",c,NULL);
}

static void lua_on_disconnect(struct connection *c,uint32_t reason)
{
	struct luaNetService *service = (struct luaNetService *)c->usr_ptr;
	service->call(service->L,service->m_iKeyIndex,"on_disconnect",c,NULL);
}

static void lua_on_accept(SOCK s,void*ud)
{
	struct luaNetService *service = (struct luaNetService *)ud;
	struct connection *c = new_conn(s,1);
	c->usr_ptr = (void*)service;
	service->net->bind(service->net,c,lua_process_packet,lua_on_disconnect
						,5000,lua_recv_timeout,5000,lua_send_timeout);
	service->call(service->L,service->m_iKeyIndex,"on_accept",c,NULL);
}

static void lua_on_connect(SOCK s,void*ud,int err)
{
	if(s != INVALID_SOCK){
		struct luaNetService *service = (struct luaNetService *)ud;
		struct connection *c = new_conn(s,1);
		c->usr_ptr = (void*)service;
		service->net->bind(service->net,c,lua_process_packet,lua_on_disconnect
							,5000,lua_recv_timeout,5000,lua_send_timeout);
		service->call(service->L,service->m_iKeyIndex,"on_connect",c,NULL);
	}
}

static int lua_active_close(lua_State *L)
{
	struct connection *c = (struct connection *)lua_touserdata(L,1);
	active_close(c);
	return 0;
}

wpacket_t luaGetluaWPacket(lua_State *L,int idx)
{
	wpacket_t wpk = wpk_create(128,1);
	wpk_write_string(wpk,lua_tostring(L,idx));
	return wpk;
}

static int luaSendPacket(lua_State *L)
{
	struct connection *c = (struct connection *)lua_touserdata(L,1);
	wpacket_t w = luaGetluaWPacket(L,2);
	if(w){	
		lua_pushnumber(L,send_packet(c,w));
	}
	else
		lua_pushnumber(L,-1);
	return 1;
}

static int luaConnect(lua_State *L)
{
	struct luaNetService *engine = (struct luaNetService *)lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	uint16_t port = (uint16_t)lua_tonumber(L,3);
	uint32_t timeout = lua_tonumber(L,4);
	engine->net->connect(engine->net,ip,port,(void*)engine,lua_on_connect,timeout);
	return 0;
}

static int luaListen(lua_State *L)
{
	struct luaNetService *engine = (struct luaNetService *)lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	uint16_t port = (uint16_t)lua_tonumber(L,3);
	if(INVALID_SOCK != engine->net->listen(engine->net,ip,port,(void*)engine,lua_on_accept))
		printf("listen ok\n");
	return 0;
}

static int luaEngineRun(lua_State *L)
{
	struct luaNetService *engine = (struct luaNetService *)lua_touserdata(L,1);
	uint32_t ms = lua_tonumber(L,2);
	if(recv_sigint)
		lua_pushnumber(L,-1);
	else
		lua_pushnumber(L,engine->net->loop(engine->net,ms));
	return 1;
}

static int luaReleaseConnection(lua_State *L)
{
	struct connection *c = (struct connection *)lua_touserdata(L,1);
	if(c)release_conn(c);
	return 0;
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
	lua_register(L,"ReleaseConnection",&luaReleaseConnection);
	InitNetSystem();
    signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
    printf("load c function finish\n");  
} 

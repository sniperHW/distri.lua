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
#include "lsock.h"
#include "core/packet.h"
#include "lua_util.h"

static int luaGetSysTick(lua_State *L){
    lua_pushnumber(L,GetSystemMs());
    return 1;
}

typedef struct nodelua
{
    msgque_t     mq_in;          //用于接收从逻辑层过来的消息
    netservice*  netpoller;      //底层的poller
    msgque_t     mq_out;
    atomic_32_t  flag;
}*nodelua_t;


static nodelua_t g_nodelua;
static thread_t  g_main_thd;
lua_State *      g_luaState;


struct msg_connection
{
    struct msg  base;
    char        ip[32];
    int32_t     port;
    uint32_t    reason;
    luaObject_t listener;
};


static void luasock_disconnect(struct connection *c,uint32_t reason)
{
    _lsock_t d = (_lsock_t)c->usr_ptr;
    struct msg_connection *msg = calloc(1,sizeof(*msg));
    MSG_TYPE(msg) = MSG_DISCONNECTED;
    MSG_USRPTR(msg) = d->luasocket;
    msg->reason = reason;
    if(0 != msgque_put_immeda(g_nodelua->mq_out,(lnode*)msg))
        free(msg);
    luasock_release(d);
}


static int8_t lua_process_packet(struct connection *c,rpacket_t r)
{
    _lsock_t ls = (_lsock_t)c->usr_ptr;
    MSG_USRPTR(r) = ls;
    if(0 != msgque_put(g_nodelua->mq_out,(lnode*)r))
        return 1;
    return 0;
}

static inline void new_connection(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
    struct connection *c = new_conn(sock,1);
    _lsock_t ls = luasock_new(c);
    struct msg_connection *msg = calloc(1,sizeof(*msg));
    MSG_TYPE(msg) = MSG_CONNECT;
    if(0 != CALL_LUA_FUNC1(g_luaState,"create_socket",1,PUSH_LUSRDATA(g_luaState,ls)))
    {
        const char * error = lua_tostring(g_luaState, -1);
        lua_pop(g_luaState,1);
        printf("%s\n",error);
    }
    ls->luasocket = create_luaObj(g_luaState);
    MSG_USRPTR(msg) = ls->luasocket;
    msg->listener = ((_lsock_t)ud)->luasocket;
    msgque_put_immeda(g_nodelua->mq_out,(lnode*)msg);
    g_nodelua->netpoller->bind(g_nodelua->netpoller,c,lua_process_packet,luasock_disconnect,
                       0,NULL,0,NULL);

}

static void accpet_callback(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
    new_connection(sock,addr_remote,ud);
}


static int luaConnect(lua_State *L)
{
    return 0;
}

static int luaListen(lua_State *L)
{
    const char *ip = lua_tostring(L,2);
    uint16_t port = (uint16_t)lua_tonumber(L,3);
    _lsock_t lsock = luasock_new(NULL);
    SOCK s = g_nodelua->netpoller->listen(g_nodelua->netpoller,ip,port,(void*)lsock,accpet_callback);
    if(INVALID_SOCK == s)
    {
        luasock_release(lsock);
        lua_pushnil(L);
        lua_pushstring(L,"listen error\n");
    }else
    {
        lsock->s = s;
        if(0 != CALL_LUA_FUNC1(L,"create_socket",1,PUSH_LUSRDATA(L,lsock)))
        {
            const char * error = lua_tostring(L, -1);
            lua_pop(L,1);
            printf("%s\n",error);
            return 0;
        }
        lsock->luasocket = create_luaObj(L);
        PUSH_LUAOBJECT(L,lsock->luasocket);
        lua_pushnil(L);
    }
    return 2;
}

_lsock_t   lua_poplsock(lua_State *L,int idx)
{
    return (_lsock_t)lua_touserdata(L,idx);
}


wpacket_t luaGetluaWPacket(lua_State *L,int idx)
{
    wpacket_t wpk = wpk_create(128,1);
    wpk_write_string(wpk,lua_tostring(L,idx));
    return wpk;
}

static int luaSendPacket(lua_State *L)
{
    _lsock_t lsock = lua_poplsock(L,1);
    if(lsock){
        wpacket_t wpk = luaGetluaWPacket(L,2);
		MSG_USRPTR(wpk)	= lsock;
        msgque_put(g_nodelua->mq_in,(lnode*)wpk);
        lua_pushnil(L);
    }else
        lua_pushstring(L,"disconnect");
    return 1;
}

static int lua_active_close(lua_State *L)
{
    _lsock_t lsock = lua_poplsock(L,1);
    if(lsock)
    {
        msg_t msg = calloc(1,sizeof(*msg));
        MSG_TYPE(msg) = MSG_ACTIVE_CLOSE;
        MSG_USRPTR(msg) = lsock;
        msgque_put_immeda(g_nodelua->mq_in,(lnode*)msg);
        lua_pushnil(L);
    }else
        lua_pushstring(L,"close error");
    return 1;
}


static void notify_function(void *arg)
{
    g_nodelua->netpoller->wakeup(g_nodelua->netpoller);
}


static void process_msg(msg_t msg)
{
    if(msg->type == MSG_ACTIVE_CLOSE)
    {
        _lsock_t ls = MSG_USRPTR(msg);
        if(ls){
            active_close(ls->c);
            luasock_release(ls);
        }
    }
    if(MSG_FN_DESTROY(msg))MSG_FN_DESTROY(msg)((void*)msg);
    else free(msg);
}


static inline void process_send(wpacket_t wpk)
{
    _lsock_t d = (_lsock_t)MSG_USRPTR(wpk);
    if(d)
    {
        send_packet(d->c,wpk);
        luasock_release(d);
    }else
    {
        //连接已失效丢弃wpk
        wpk_destroy(&wpk);
    }
}


static void *node_mainloop(void *arg)
{
    printf("start io thread\n");
    while(0 == g_nodelua->flag)
    {
        uint32_t tick = GetSystemMs();
        uint32_t timeout = tick + 50;
        int8_t is_empty = 0;
        for(;tick < timeout;){
            lnode *node = NULL;
            msgque_get(g_nodelua->mq_in,&node,0);
            if(node)
            {
                msg_t _msg = (msg_t)node;
                if(MSG_TYPE(_msg) == MSG_WPACKET)
                    process_send((wpacket_t)_msg);
                else
                    process_msg(_msg);
            }
            else{
                is_empty = 1;
                break;
            }
            tick = GetSystemMs();
        }
        if(is_empty){
            //注册中断器，如果阻塞在loop里时mq_in收到消息会调用唤醒函数唤醒loop
            msgque_putinterrupt(g_nodelua->mq_in,NULL,notify_function);
            g_nodelua->netpoller->loop(g_nodelua->netpoller,50);
            msgque_removeinterrupt(g_nodelua->mq_in);
        }
        else
            g_nodelua->netpoller->loop(g_nodelua->netpoller,0);
    }
    return NULL;
}


void lua_pushmsg(lua_State *L,msg_t msg){
    if(MSG_TYPE(msg) == MSG_RPACKET)
    {
        rpacket_t rpk = (rpacket_t)msg;
        _lsock_t ls = (_lsock_t)MSG_USRPTR(msg);
        PUSH_TABLE3(L,PUSH_LUAOBJECT(L,ls->luasocket),
                      PUSH_STRING(L,"packet"),
                      PUSH_STRING(L,rpk_read_string(rpk))
                    );

        rpk_destroy((rpacket_t*)&msg);
    }else
    {
        if(MSG_TYPE(msg) == MSG_CONNECT)
        {
            struct msg_connection *_msg = (struct msg_connection*)msg;
            luaObject_t lo = (luaObject_t)MSG_USRPTR(_msg);
            PUSH_TABLE3(L,PUSH_LUAOBJECT(L,(_msg->listener)),
                          PUSH_STRING(L,"newconnection"),
                          PUSH_LUAOBJECT(L,lo)
                        );
        }else if(MSG_TYPE(msg) == MSG_DISCONNECTED)
        {
            struct msg_connection *_msg = (struct msg_connection*)msg;
            luaObject_t lo = (luaObject_t)MSG_USRPTR(_msg);
            PUSH_TABLE3(L,PUSH_LUAOBJECT(L,lo),
                          PUSH_STRING(L,"disconnected"),
                          PUSH_NUMBER(L,_msg->reason));
        }
        else
            PUSH_NIL(L);
        free(msg);
    }
}


int lua_node_peekmsg(lua_State *L)
{
    int ms = (int)lua_tonumber(L,1);
    lnode *n;
    if(0 != msgque_get(g_nodelua->mq_in,&n,ms)){
        lua_pushnil(L);
        lua_pushstring(L,"peek error");
    }
    else{
        if(n){
            lua_pushmsg(L,(msg_t)n);
            lua_pushnil(L);
        }else{
            lua_pushnil(L);
            lua_pushstring(L,"timeout");
        }
    }
    return 2;
}

static void mq_item_destroyer(void *ptr)
{
    msg_t _msg = (msg_t)ptr;
    if(MSG_TYPE(_msg) == MSG_RPACKET)
        rpk_destroy((rpacket_t*)&_msg);
    else if(MSG_TYPE(_msg) == MSG_WPACKET)
        wpk_destroy((wpacket_t*)&_msg);
    else
    {
        if(MSG_FN_DESTROY(_msg))
            MSG_FN_DESTROY(_msg)(ptr);
        else
            free(ptr);
    }
}

void RegisterNet(lua_State *L){

    lua_register(L,"Listen",&luaListen);
    lua_register(L,"Connect",&luaConnect);
    lua_register(L,"Close",&lua_active_close);
    lua_register(L,"GetSysTick",&luaGetSysTick);
    lua_register(L,"SendPacket",&luaSendPacket);
    lua_register(L,"PeekMsg",&lua_node_peekmsg);
    InitNetSystem();

    g_nodelua = calloc(1,sizeof(*g_nodelua));
    g_main_thd = create_thread(THREAD_JOINABLE);
    g_nodelua->mq_in = new_msgque(1024,mq_item_destroyer);
    g_nodelua->mq_out = new_msgque(1024,mq_item_destroyer);
    g_nodelua->netpoller = new_service();
    g_luaState = L;
    thread_start_run(g_main_thd,node_mainloop,NULL);
    //signal(SIGINT,sig_int);
    signal(SIGPIPE,SIG_IGN);
    printf("load c function finish\n");
}

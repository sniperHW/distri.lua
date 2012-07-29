#include "lua.h"  
#include "lauxlib.h"  
#include "lualib.h"  
#include "link_list.h"
#include "KendyNet.h"
#include "Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include "SocketWrapper.h"
#include "Acceptor.h"
#include "Connector.h"
#include "wpacket.h"

uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t s_p = 0;
uint32_t bf_count = 0;
uint32_t clientcount = 0;
uint32_t last_send_tick = 0;
uint32_t recv_count = 0;

void BindFunction(lua_State *lState);  
void RegisterNet(lua_State *L)  
{  
    BindFunction(L);  
}

extern void SendFinish(int32_t bytetransfer,st_io *io);
extern void RecvFinish(int32_t bytetransfer,st_io *io);

struct luaNetEngine
{
	HANDLE engine;
	acceptor_t _acceptor;
	struct link_list *msgqueue;
};

struct luaconnection
{
	struct connection connection;
	struct luaNetEngine *engine;
};

struct luaNetMsg
{
	list_node next;
	struct luaconnection *connection;
	rpacket_t packet;
	int8_t    msgType;//1,新连接;2,连接断开，3，网络消息
};


void on_process_packet(struct connection *c,rpacket_t r)
{
	struct luaconnection *con = (struct luaconnection *)c;
	struct luaNetMsg *msg = (struct luaNetMsg *)calloc(1,sizeof(*msg));
	msg->msgType = 3;
	msg->connection = con;
	msg->packet = r;
	LINK_LIST_PUSH_BACK(con->engine->msgqueue,msg);
}

void _on_disconnect(struct connection *c)
{
	struct luaconnection *con = (struct luaconnection *)c;
	struct luaNetMsg *msg = (struct luaNetMsg *)calloc(1,sizeof(*msg));
	msg->msgType = 2;
	msg->connection = con;
	LINK_LIST_PUSH_BACK(con->engine->msgqueue,msg);
}

struct luaconnection* createluaconnection()
{
	struct luaconnection *c = calloc(1,sizeof(*c));;
	c->connection.send_list = LINK_LIST_CREATE();
	c->connection._process_packet = on_process_packet;
	c->connection._on_disconnect = _on_disconnect;
	c->connection.next_recv_buf = 0;
	c->connection.next_recv_pos = 0;
	c->connection.unpack_buf = 0;
	c->connection.unpack_pos = 0;
	c->connection.unpack_size = 0;
	c->connection.recv_overlap.c = (struct connection*)c;
	c->connection.send_overlap.c = (struct connection*)c;
	c->connection.raw = 1;
	c->connection.mt = 0;
	return c;
}


//connect to remote server,return a connection for future recv and send
int luaConnect(lua_State *L)
{
	struct luaNetEngine *engine = (struct luaNetEngine*)lua_touserdata(L,1);
	const char *ip = lua_tostring(L,2);
	uint16_t port = (uint16_t)lua_tonumber(L,3);
	struct sockaddr_in remote;
	HANDLE sock;
	sock = OpenSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0)
	{
		lua_pushnil(L);
		return 1;
	}
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	if(inet_pton(INET,ip,&remote.sin_addr) < 0)
	{
		printf("%s\n",strerror(errno));
		lua_pushnil(L);
		return 1;
	}
	if(Connect(sock, (struct sockaddr *)&remote, sizeof(remote)) != 0)
	{
		lua_pushnil(L);
		return 1;
	}
	struct luaconnection *c = createluaconnection();
	c->connection.socket = sock;
	c->engine = engine;
	setNonblock(sock);
	Bind2Engine(engine->engine,sock,RecvFinish,SendFinish);
	lua_pushlightuserdata(L,(void*)c);
	return 1;
}

int luaCloseConnection(lua_State *L)
{
	struct luaconnection *c = lua_touserdata(L,1);
	if(c)
	{
		ReleaseSocketWrapper(c->connection.socket);
		wpacket_t w;
		while(w = LINK_LIST_POP(wpacket_t,c->connection.send_list))
			wpacket_destroy(&w);
		LINK_LIST_DESTROY(&(c->connection.send_list));
		buffer_release(&(c->connection.unpack_buf));
		buffer_release(&(c->connection.next_recv_buf));
		free(c);
	}
	return 0;
}

void accept_callback(HANDLE s,void *ud)
{
	struct luaNetEngine *engine = (struct luaNetEngine *)ud;
	struct luaconnection *c = createluaconnection();
	c->connection.socket = s;
	c->engine = engine;
	setNonblock(s);	
	struct luaNetMsg *msg = (struct luaNetMsg *)calloc(1,sizeof(*msg));
	msg->msgType = 1;
	msg->connection = c;
	msg->packet = NULL;
	LINK_LIST_PUSH_BACK(engine->msgqueue,msg);
	connection_recv((struct connection*)c);
	Bind2Engine(engine->engine,s,RecvFinish,SendFinish);
		
}

int luaCreateNet(lua_State *L)
{
	const char *ip = lua_tostring(L,1);
	uint16_t port = (uint16_t)lua_tonumber(L,2);
	struct luaNetEngine *e = (struct luaNetEngine *)calloc(1,sizeof(*e));
	e->msgqueue = LINK_LIST_CREATE();
	e->_acceptor = create_acceptor(ip,port,&accept_callback,e);
	e->engine = CreateEngine();
	lua_pushlightuserdata(L,e);
	return 1;
}

int luaPeekMsg(lua_State *L)
{
	struct luaNetEngine *engine = (struct luaNetEngine *)lua_touserdata(L,1);
	uint32_t ms = lua_tonumber(L,2);
	acceptor_run(engine->_acceptor,1);
	//if(link_list_is_empty(engine->msgqueue))
	if(-1 == EngineRun(engine->engine,ms))
			printf("error\n");
	struct luaNetMsg *msg = (struct luaNetMsg *)link_list_pop(engine->msgqueue);
	if(msg)
	{
		lua_pushnumber(L,msg->msgType);
		lua_pushlightuserdata(L,msg->connection);
		if(msg->packet)
			lua_pushlightuserdata(L,msg->packet);
		else
			lua_pushnil(L);
		free(msg);
		return 3;
	}
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);
	return 3;
}

int luaCreateWpacket(lua_State *L)
{
	rpacket_t r = lua_touserdata(L,1);
	wpacket_t w;
	if(r)
		w = wpacket_create_by_rpacket(NULL,r);
	else
	{
		uint32_t size = lua_tonumber(L,2);
		w = wpacket_create(0,NULL,size,0);	
	}
	lua_pushlightuserdata(L,w);
	return 1;
}

int luaReleaseRpacket(lua_State *L)
{
	rpacket_t r = lua_touserdata(L,1);
	rpacket_destroy(&r);
	return 0;
}

int luaSendPacket(lua_State *L)
{
	struct luaconnection *c = (struct luaconnection *)lua_touserdata(L,1);
	wpacket_t w = (wpacket_t)lua_touserdata(L,2);
	lua_pushnumber(L,connection_send(&(c->connection),w,0));
	return 1;
}

int luaPacketReadString(lua_State *L)
{
	rpacket_t r = (rpacket_t)lua_touserdata(L,1);
	const char *str = rpacket_read_string(r);
	if(!str)
		lua_pushnil(L);
	else
		lua_pushstring(L,str);
	return 1;
}

void BindFunction(lua_State *lState)  
{  
    lua_register(lState,"Connect",&luaConnect);  
    lua_register(lState,"CloseConnection",&luaCloseConnection);  
    lua_register(lState,"CreateNet",&luaCreateNet);  
    lua_register(lState,"PeekMsg",&luaPeekMsg);  
    lua_register(lState,"CreateWpacket",&luaCreateWpacket);  
    lua_register(lState,"ReleaseRpacket",&luaReleaseRpacket);
    lua_register(lState,"SendPacket",&luaSendPacket);
    lua_register(lState,"PacketReadString",&luaPacketReadString);
    InitNetSystem();
    printf("load c function finish\n");    
} 

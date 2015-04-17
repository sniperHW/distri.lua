#include "connection.h"
#include "luapacket.h"
#include "luasocket.h"
#include "http-parser/http_parser.h"
#include "lua/lua_util.h"

enum{ 
	  ON_MESSAGE_BEGIN = 0,
	  ON_URL,
	  ON_STATUS,
	  ON_HEADER_FIELD,
	  ON_HEADER_VALUE,
	  ON_HEADERS_COMPLETE,
	  ON_BODY,
	  ON_MESSAGE_COMPLETE,
};

const char *event_type[] = {
	  "ON_MESSAGE_BEGIN",
	  "ON_URL",
	  "ON_STATUS",
	  "ON_HEADER_FIELD",
	  "ON_HEADER_VALUE",
	  "ON_HEADERS_COMPLETE",
	  "ON_BODY",
	  "ON_MESSAGE_COMPLETE",
};

struct luahttp_parser{
	http_parser base;
	http_parser_settings settings;	
	connection_t c;
	uint32_t maxsize;
	buffer_t buf;
	int parser_type;	
};

struct httpdecoder{
	decoder base;
	http_parser_settings settings;
	struct luahttp_parser parser;
};

#define HTTPEVENT_METATABLE "httpevent_metatable"

struct httpevent{
	const char *event;
	struct luahttp_parser *parser;
	uint32_t pos;
	uint32_t len;
};

inline static struct httpevent* lua_gethttpevent(lua_State *L, int index) {
    return (struct httpevent*)luaL_testudata(L, index, HTTPEVENT_METATABLE);
}

static int lua_get_method(lua_State *L){
	struct httpevent *ev = lua_gethttpevent(L,1);
	ev ? lua_pushstring(L,http_method_str(((http_parser*)ev->parser)->method)) : lua_pushnil(L);
	return 1;
}

static int lua_get_status(lua_State *L){
	struct httpevent *ev = lua_gethttpevent(L,1);
	ev ? lua_pushinteger(L,((http_parser*)ev->parser)->status_code) : lua_pushnil(L);
	return 1;
}

static int lua_get_content(lua_State *L){
	struct httpevent *ev = lua_gethttpevent(L,1);
	if(!ev || !ev->len) 
		lua_pushnil(L);
	else
		lua_pushlstring(L,(char*)&ev->parser->buf->buf[ev->pos],(size_t)ev->len);	
	return 1;
}

static int lua_get_evtype(lua_State *L){
	struct httpevent *ev = lua_gethttpevent(L,1);
	ev ? lua_pushstring(L,ev->event) : lua_pushnil(L);
	return 1;	
}

static void create_httpevent(lua_State *L,struct luahttp_parser *parser,const char *evtype,uint32_t pos,uint32_t len){
		struct httpevent *ev = (struct httpevent*)lua_newuserdata(L, sizeof(*ev));
		luaL_getmetatable(L, HTTPEVENT_METATABLE);
		lua_setmetatable(L, -2);
		ev->parser = parser;
		ev->event = evtype;
		ev->pos = pos;
		ev->len = len;
}

void  on_http_cb(connection_t c,packet_t p){
	luasocket_t luasock = (luasocket_t)connection_getud(c);
	luaRef_t  *obj = &luasock->luaObj;	
	int __result;
	lua_State *__L = obj->L;
	int __oldtop = lua_gettop(__L);
	lua_rawgeti(__L,LUA_REGISTRYINDEX,obj->rindex);
	lua_pushstring(__L,"__on_packet");
	lua_gettable(__L,-2);
	lua_insert(__L,-2);	
	struct httpevent *ev = (struct httpevent*)p;
	create_httpevent(__L,ev->parser,ev->event,ev->pos,ev->len);
	__result = lua_pcall(__L,2,0,0);
	if(__result){
		const char *error = lua_tostring(__L,-1);
		if(error){
			SYS_LOG(LOG_ERROR,"error on __on_packet:%s\n",error);
		}	
	}
	lua_settop(__L,__oldtop);	
	//return 0;	
}

static inline int _http_data_cb(http_parser *_parser, const char *at, size_t length,int evtype){
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	struct httpevent tmp = {
		.event = event_type[evtype],
		.parser = parser,
		.pos = (uint32_t)(at - (const char*)parser->buf->buf),
		.len = length,
	};
	parser->c->on_packet(parser->c,(packet_t)&tmp);
	return 0;	
}

static inline int _http_cb(http_parser *_parser,int evtype){
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	struct httpevent tmp = {
		.event = event_type[evtype],
		.parser = parser,
		.pos = 0,
		.len = 0,
	};
	parser->c->on_packet(parser->c,(packet_t)&tmp);
	return 0;
}


static int on_message_begin (http_parser *_parser){
	return _http_cb(_parser,ON_MESSAGE_BEGIN);
}

static int on_url(http_parser *_parser, const char *at, size_t length){	
	return _http_data_cb(_parser,at,length,ON_URL);	
}

static int on_status(http_parser *_parser, const char *at, size_t length){
	return _http_data_cb(_parser,at,length,ON_STATUS);				
}

static int on_header_field(http_parser *_parser, const char *at, size_t length){
	return _http_data_cb(_parser,at,length,ON_HEADER_FIELD);				
}

static int on_header_value(http_parser *_parser, const char *at, size_t length){
	return _http_data_cb(_parser,at,length,ON_HEADER_VALUE);			
}

static int on_headers_complete(http_parser *_parser){	
	return _http_cb(_parser,ON_HEADERS_COMPLETE);		
}

static int on_body(http_parser *_parser, const char *at, size_t length){
	return _http_data_cb(_parser,at,length,ON_BODY);			
}

static int on_message_complete(http_parser *_parser){	
	int ret = _http_cb(_parser,ON_MESSAGE_COMPLETE);
	((struct luahttp_parser*)_parser)->buf->size = 0;
	return ret;			
}

static int unpack(decoder *d,void *_){
	uint32_t pk_len = 0;
	connection_t c = (connection_t)_;
	struct httpdecoder *httpd = (struct httpdecoder*)d;
	if(!httpd->parser.c) httpd->parser.c = c;
	do{ 
		pk_len = c->unpack_buf->size - c->unpack_pos;
		if(!pk_len) return 0;
		
		if(pk_len + httpd->parser.buf->size > httpd->parser.maxsize) 
			return decode_packet_too_big;
			
		uint32_t idx = httpd->parser.buf->size;		
		buffer_write(httpd->parser.buf,idx,&c->unpack_buf->buf[c->unpack_pos],pk_len);		
		httpd->parser.buf->size += pk_len;
		size_t nparsed = http_parser_execute((http_parser*)&httpd->parser,&httpd->parser.settings,(char*)&httpd->parser.buf->buf[idx],pk_len);		
		if(nparsed != pk_len)
			return decode_unknow_error;									
		c->unpack_pos  += pk_len;
		c->unpack_size -= pk_len;
		if(c->unpack_pos >= c->unpack_buf->capacity)
		{
			assert(c->unpack_buf->next);
			c->unpack_pos = 0;
			c->unpack_buf = buffer_acquire(c->unpack_buf,c->unpack_buf->next);
		}
		if(connection_isclose(c)) return decode_socket_close;
	}while(1);
	return 0;
}

static void http_decoder_destroy(struct decoder *d){
	buffer_release(((struct httpdecoder*)d)->parser.buf);
}


int mask_http_decode = 19820702;

int new_http_decoder(lua_State *L){
	int parser_type = lua_tointeger(L,1);
	uint32_t maxsize = lua_tointeger(L,2);	
	struct httpdecoder *d = calloc(1,sizeof(*d));
	d->base.mask = mask_http_decode;
	d->base.destroy = http_decoder_destroy;
	d->parser.settings.on_message_begin = on_message_begin;
	d->parser.settings.on_url = on_url;
	d->parser.settings.on_status = on_status;
	d->parser.settings.on_header_field = on_header_field;
	d->parser.settings.on_header_value = on_header_value;
	d->parser.settings.on_headers_complete = on_headers_complete;
	d->parser.settings.on_body = on_body;
	d->parser.settings.on_message_complete = on_message_complete;
	d->parser.maxsize = maxsize;
	d->parser.buf = buffer_create(maxsize);
	d->base.unpack = unpack;
	d->parser.parser_type = parser_type;
	http_parser_init((http_parser*)&d->parser,parser_type);
	lua_pushlightuserdata(L,d);
	return 1;
}

#define REGISTER_CONST(___N) do{\
		lua_pushstring(L, #___N);\
		lua_pushinteger(L, ___N);\
		lua_settable(L, -3);\
}while(0)

#define REGISTER_FUNCTION(NAME,FUNC) do{\
	lua_pushstring(L,NAME);\
	lua_pushcfunction(L,FUNC);\
	lua_settable(L, -3);\
}while(0)

void reg_luahttp(lua_State *L){
    luaL_Reg httpevent_methods[] = {
        {"Method", lua_get_method},
        {"Status", lua_get_status},
        {"Event", lua_get_evtype},
        {"Content", lua_get_content},
        {NULL, NULL}
    };

    luaL_newmetatable(L, HTTPEVENT_METATABLE);
    luaL_newlib(L, httpevent_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);	
	
	lua_newtable(L);
	REGISTER_FUNCTION("http_decoder",new_http_decoder);	
	REGISTER_CONST(HTTP_REQUEST);
	REGISTER_CONST(HTTP_RESPONSE);
	REGISTER_CONST(HTTP_BOTH);			
	lua_setglobal(L,"CHttp");
}






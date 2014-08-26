#include "stream_conn.h"
#include "luapacket.h"
#include "http-parser/http_parser.h"
#include "lua_util.h"

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
	stream_conn_t c;
	uint32_t maxsize;
	buffer_t buf;
	int parser_type;	
};

struct httpdecoder{
	decoder base;
	http_parser_settings settings;
	struct luahttp_parser parser;
};


static  httppacket_t httppacket_create(buffer_t buf,uint32_t begpos,uint32_t len,uint8_t ev_type){
    httppacket_t p = calloc(1,sizeof(*p));
    p->ev_type = event_type[ev_type];
	packet_type(p) = HTTPPACKET;    
    if(buf){
		packet_buf(p) = buf;
		packet_begpos(p) = begpos;
		packet_datasize(p) = len;
	}
    return p;    	
}



static int on_message_begin (http_parser *_parser){
	//printf("on_message_begin\n");
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;	
	httppacket_t p = httppacket_create(NULL,0,0,ON_MESSAGE_BEGIN);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_url(http_parser *_parser, const char *at, size_t length){
	//printf("on_url\n");
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(parser->buf,(uint32_t)(at - (const char*)parser->buf->buf),length,ON_URL);
	parser->c->on_packet(parser->c,(packet_t)p);	
	return 0;	
}

static int on_status(http_parser *_parser, const char *at, size_t length){
	//printf("on_status\n");	
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(parser->buf,(uint32_t)(at - (const char*)parser->buf->buf),length,ON_STATUS);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_header_field(http_parser *_parser, const char *at, size_t length){
	//printf("on_header_field\n");		
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(parser->buf,(uint32_t)(at - (const char*)parser->buf->buf),length,ON_HEADER_FIELD);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_header_value(http_parser *_parser, const char *at, size_t length){
	//printf("on_header_value\n");		
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(parser->buf,(uint32_t)(at - (const char*)parser->buf->buf),length,ON_HEADER_VALUE);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_headers_complete(http_parser *_parser){
	//printf("on_headers_complete\n");	
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(NULL,0,0,ON_HEADERS_COMPLETE);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_body(http_parser *_parser, const char *at, size_t length){
	//printf("on_body\n");		
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(parser->buf,(uint32_t)(at - (const char*)parser->buf->buf),length,ON_BODY);
	parser->c->on_packet(parser->c,(packet_t)p);
	return 0;
}

static int on_message_complete(http_parser *_parser){
	//printf("on_message_complete\n");	
	struct luahttp_parser *parser = (struct luahttp_parser*)_parser;
	httppacket_t p = httppacket_create(NULL,0,0,ON_MESSAGE_COMPLETE);
	parser->c->on_packet(parser->c,(packet_t)p);
	//printf("%d:%s\n",cc,parser->buf->buf);
	parser->buf->size = 0;
	http_parser_init(_parser,parser->parser_type);	
	return 0;	
}

static int unpack(decoder *d,stream_conn_t c){
	uint32_t pk_len = 0;
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
		if(c->is_close) return decode_socket_close;
	}while(1);
	return 0;
}

static void http_decoder_destroy(struct decoder *d){
	buffer_release(((struct httpdecoder*)d)->parser.buf);
}

int new_http_decoder(lua_State *L){
	int parser_type = lua_tointeger(L,1);
	uint32_t maxsize = lua_tointeger(L,2);	
	struct httpdecoder *d = calloc(1,sizeof(*d));
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
	lua_newtable(L);
	REGISTER_FUNCTION("http_decoder",new_http_decoder);	
	REGISTER_CONST(HTTP_REQUEST);
	REGISTER_CONST(HTTP_RESPONSE);
	REGISTER_CONST(HTTP_BOTH);			
	lua_setglobal(L,"CHttp");	
}






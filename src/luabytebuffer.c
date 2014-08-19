#include "luabytebuffer.h"

//原始的bytebuffer,请在lua中再做一层封装

static int lua_new_bytebuffer(lua_State *L){
	const char *data = NULL;
	size_t      len  = 0; 	
	if(lua_type(L,1) == LUA_TSTRING){
		data = lua_tolstring(L,1,&len);
	}else if(lua_type(L,1) == LUA_TNUMBER){
		len = size_of_pow2(lua_tointeger(L,1));
		if(len < 64) len = 64;
	}else{
		return luaL_error(L,"arg should be integer or string");
	}			
	lua_bytebuffer_t bbuffer = (lua_bytebuffer_t)lua_newuserdata(L, sizeof(lua_bytebuffer_t));
    luaL_getmetatable(L, BYTEBUFFER_METATABLE);
    lua_setmetatable(L, -2);	
	bbuffer->raw_buffer = buffer_create(len);
	if(data){
		buffer_write(bbuffer->raw_buffer,0,(int8_t*)data,len);
		bbuffer->raw_buffer->size += len;
	}
	return 1;
}

inline static lua_bytebuffer_t lua_getbytebuffer(lua_State *L, int index) {
    return (lua_bytebuffer_t)luaL_testudata(L, index, BYTEBUFFER_METATABLE);
}

static int destroy_bytebuffer(lua_State *L) {
	lua_bytebuffer_t bbuffer = lua_getbytebuffer(L,1);
	buffer_release(bbuffer->raw_buffer);
    return 0;
}


#define ARG_CHECK\
	lua_bytebuffer_t bbuffer = lua_getbytebuffer(L,1);\
	if(!bbuffer) return luaL_error(L,"arg 1 must be bytebuffer");\
	if(lua_type(L,2) != LUA_TNUMBER) return luaL_error(L,"arg 2 must be integer");\
	uint32_t pos = lua_tointeger(L,2)

static int lua_bytebuffer_read_raw(lua_State *L){
	ARG_CHECK;
	int len = bbuffer->raw_buffer->size;
	if(lua_gettop(L) == 3 && lua_type(L,3) == LUA_TNUMBER)
		len = lua_tointeger(L,3);
	lua_pushlstring(L,(const char *)&bbuffer->raw_buffer->buf[pos],len);
	return 1;
}

static int lua_bytebuffer_read_string(lua_State *L){
	ARG_CHECK;	
	uint32_t len;
	if(0 != buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&len,sizeof(len)))
		lua_pushnil(L);
	else{
		pos += 4;
		lua_pushlstring(L,(const char *)&bbuffer->raw_buffer->buf[pos],len);
	}
	return 1;
}

static int lua_bytebuffer_read_uint8(lua_State *L){
	ARG_CHECK;
	uint8_t val;
	buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&val,sizeof(val));
	lua_pushinteger(L,val);
	return 1;
}

static int lua_bytebuffer_read_uint16(lua_State *L){
	ARG_CHECK;
	uint16_t val;
	buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&val,sizeof(val));
	lua_pushinteger(L,val);
	return 1;
}

static int lua_bytebuffer_read_uint32(lua_State *L){
	ARG_CHECK;
	uint32_t val;
	buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&val,sizeof(val));
	lua_pushinteger(L,val);
	return 1;
}

static int lua_bytebuffer_read_float(lua_State *L){
	ARG_CHECK;
	float val;
	buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&val,sizeof(val));
	lua_pushnumber(L,val);
	return 1;
}

static int lua_bytebuffer_read_double(lua_State *L){
	ARG_CHECK;
	double val;
	buffer_read(bbuffer->raw_buffer,pos,(int8_t*)&val,sizeof(val));
	lua_pushnumber(L,val);
	return 1;
}


#define SIZE_CHECK\
	if(len + pos > bbuffer->raw_buffer->capacity){\
		size_t size = size_of_pow2(len + pos);\
		buffer_t newbuffer = buffer_create(size);\
		memcpy(newbuffer->buf,bbuffer->raw_buffer->buf,bbuffer->raw_buffer->size);\
		buffer_release(bbuffer->raw_buffer);\
		bbuffer->raw_buffer = newbuffer;\
	}
	
static int lua_bytebuffer_write_raw(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TSTRING) return luaL_error(L,"arg 3 must be string");
	size_t len;
	const char *bin = lua_tolstring(L,3,&len);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)bin,len);
	bbuffer->raw_buffer->size = pos + len;
	return 0;	
}

static int lua_bytebuffer_write_string(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TSTRING) return luaL_error(L,"arg 3 must be string");
	size_t len;	
	const char *bin = lua_tolstring(L,3,&len);
	uint32_t strlen = len;
	len += sizeof(uint32_t);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&strlen,sizeof(strlen));
	pos += sizeof(strlen);
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)bin,strlen);	
	return 0;	
}

static int lua_bytebuffer_write_uint8(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	uint8_t val = lua_tointeger(L,3);
	size_t len;	
	len = sizeof(val);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&val,len);	
	return 0;	
}

static int lua_bytebuffer_write_uint16(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	uint16_t val = lua_tointeger(L,3);
	size_t len;	
	len = sizeof(val);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&val,len);	
	return 0;	
}

static int lua_bytebuffer_write_uint32(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	uint32_t val = lua_tointeger(L,3);
	size_t len;	
	len = sizeof(val);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&val,len);	
	return 0;	
}

static int lua_bytebuffer_write_float(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	float val = lua_tonumber(L,3);
	size_t len;	
	len = sizeof(val);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&val,len);	
	return 0;	
}

static int lua_bytebuffer_write_double(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	double val = lua_tonumber(L,3);
	size_t len;	
	len = sizeof(val);
	SIZE_CHECK;
	buffer_write(bbuffer->raw_buffer,pos,(int8_t*)&val,len);	
	return 0;	
}

static int lua_bytebuffer_size(lua_State *L){
	lua_bytebuffer_t bbuffer = lua_getbytebuffer(L,1);
	if(!bbuffer) return luaL_error(L,"arg 1 must be bytebuffer");
	lua_pushinteger(L,bbuffer->raw_buffer->size);	
	return 1;
}

static int lua_bytebuffer_cap(lua_State *L){
	lua_bytebuffer_t bbuffer = lua_getbytebuffer(L,1);
	if(!bbuffer) return luaL_error(L,"arg 1 must be bytebuffer");
	lua_pushinteger(L,bbuffer->raw_buffer->capacity);	
	return 1;
}

static int lua_bytebuffer_updatesize(lua_State *L){
	lua_bytebuffer_t bbuffer = lua_getbytebuffer(L,1);
	if(!bbuffer) return luaL_error(L,"arg 1 must be bytebuffer");
	if(lua_type(L,2) != LUA_TNUMBER) return luaL_error(L,"arg 2 must be number");
	bbuffer->raw_buffer->size = lua_tointeger(L,2);
	return 0;	
}


//将pos开始的len个字节移动到0开始的位置
static int lua_bytebuffer_move(lua_State *L){
	ARG_CHECK;	
	if(lua_type(L,3) != LUA_TNUMBER) return luaL_error(L,"arg 3 must be number");
	int len = lua_tointeger(L,3);
	//if(pos + len > pos)
	memmove(&bbuffer->raw_buffer->buf[0],&bbuffer->raw_buffer->buf[pos],len);
	return 0;	
}

void reg_luabytebuffer(lua_State *L){
//int luaopen_luabytebuffer_c(lua_State *L){
    luaL_Reg bytebuffer_mt[] = {
        {"__gc", destroy_bytebuffer},
        {NULL, NULL}
    };

    luaL_Reg bytebuffer_methods[] = {
        {"read_uint8", lua_bytebuffer_read_uint8},
        {"read_uint16", lua_bytebuffer_read_uint16},
        {"read_uint32", lua_bytebuffer_read_uint32},
        {"read_float", lua_bytebuffer_read_float},
        {"read_double", lua_bytebuffer_read_double},        
        {"read_string", lua_bytebuffer_read_string},
        {"read_raw", lua_bytebuffer_read_raw},       
        
        {"write_uint8", lua_bytebuffer_write_uint8},
        {"write_uint16", lua_bytebuffer_write_uint16},
        {"write_uint32", lua_bytebuffer_write_uint32},
        {"write_float", lua_bytebuffer_write_float},
        {"write_double", lua_bytebuffer_write_double},        
        {"write_string", lua_bytebuffer_write_string},
        {"write_raw", lua_bytebuffer_write_raw},

        {"size", lua_bytebuffer_size},
        {"cap", lua_bytebuffer_cap},
        {"updatesize", lua_bytebuffer_updatesize}, 
        {"move",lua_bytebuffer_move},
        {NULL, NULL}
    };

    luaL_newmetatable(L, BYTEBUFFER_METATABLE);
    luaL_setfuncs(L, bytebuffer_mt, 0);

    luaL_newlib(L, bytebuffer_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_Reg l[] = {
        {"bytebuffer", lua_new_bytebuffer},
        {NULL, NULL}
    };
    luaL_newlib(L, l);
	lua_setglobal(L,"CBuffer");    	
}


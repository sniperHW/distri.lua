#include "luapacket.h"

static int lua_new_packet(lua_State *L,int type){
	if(lua_type(L,1) == LUA_TNUMBER){
		//参数为数字,构造一个初始大小为len的wpacket
		size_t len = size_of_pow2(lua_tointeger(L,1));
		if(len < 64) len = 64;
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
		luaL_getmetatable(L, LUAPACKET_METATABLE);
		lua_setmetatable(L, -2);
		p->_packet = (packet_t)wpk_create(len);
		return 1;
	}else if(lua_type(L,1) == LUA_TSTRING){
		//参数为string,构造一个函数数据data的rawpacket
		size_t len;
		char *data = (char*)lua_tolstring(L,1,&len);
		lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
		luaL_getmetatable(L, LUAPACKET_METATABLE);
		lua_setmetatable(L, -2);				
		p->_packet = (packet_t)rawpacket_create2(data,len);
		return 1;				
	}else if(lua_type(L,1) == LUA_TUSERDATA){
		//参数为lua_packet_t,拷贝构造
		lua_packet_t other = lua_getluapacket(L,1);
		if(!other)
			return luaL_error(L,"invaild opration for arg1");
		if(type == WPACKET){
			if(other->_packet->type == RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)wpk_copy_create(other->_packet);
			return 1;			
		}else if(type == RPACKET){
			if(other->_packet->type == RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)rpk_copy_create(other->_packet);
			return 1;				
		}else if(type == RAWPACKET){
			if(other->_packet->type != RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)rawpacket_copy_create((rawpacket_t)other->_packet);
			return 1;									
		}else
			return luaL_error(L,"invaild opration for arg2");			
	}else
		return luaL_error(L,"invaild opration for arg1");
}

void push_luapacket(lua_State *L,packet_t pk){
	lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
	luaL_getmetatable(L, LUAPACKET_METATABLE);
	lua_setmetatable(L, -2);
	p->_packet = pk;
}


static int destroy_luapacket(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet){ 
		if(p->_packet->type != HTTPPACKET)
			destroy_packet(p->_packet);
		else{
			buffer_release(packet_buf(p->_packet));
			free(p->_packet);
		}
	}
    return 0;
}


static int _write_uint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	uint8_t v = (uint8_t)lua_tointeger(L,2);
	wpacket_t wpk = (wpacket_t)p->_packet;
	wpk_write_uint8(wpk,v);	
	return 0;	
}

static int _write_uint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	uint16_t v = (uint16_t)lua_tointeger(L,2);
	wpacket_t wpk = (wpacket_t)p->_packet;
	wpk_write_uint16(wpk,v);	
	return 0;	
}

static int _write_uint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	uint32_t v = (uint32_t)lua_tointeger(L,2);
	wpacket_t wpk = (wpacket_t)p->_packet;
	wpk_write_uint32(wpk,v);	
	return 0;	
}

static int _write_double(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg2");
	double v = (double)lua_tonumber(L,2);
	wpacket_t wpk = (wpacket_t)p->_packet;
	wpk_write_double(wpk,v);	
	return 0;	
}

static int _write_string(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TSTRING)
		return luaL_error(L,"invaild arg2");
	size_t len;
	const char *data = lua_tolstring(L,2,&len);
	wpacket_t wpk = (wpacket_t)p->_packet;
	wpk_write_binary(wpk,data,len);
	return 0;	
}

static int _read_uint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,rpk_read_uint8(rpk));
	return 1;	
}

static int _read_uint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,rpk_read_uint16(rpk));
	return 1;	
}

static int _read_uint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,rpk_read_uint32(rpk));
	return 1;	
}


static int _read_double(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushnumber(L,rpk_read_double(rpk));
	return 1;	
}

static int _read_string(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	uint32_t len;
	const char *data = rpk_read_binary(rpk,&len);
	lua_pushlstring(L,data,(size_t)len);
	return 1;
}

static int _read_rawbin(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet)// || p->_packet->type != RAWPACKET)
		return luaL_error(L,"invaild opration");
	if(p->_packet->type == RAWPACKET){
		rawpacket_t rawpk = (rawpacket_t)p->_packet;
		uint32_t len = 0;
		const char *data = rawpacket_data(rawpk,&len);
		lua_pushlstring(L,data,(size_t)len);
		return 1;
	}else if(p->_packet->type == HTTPPACKET){
		if(packet_datasize(p->_packet)){
			uint32_t begpos = packet_begpos(p->_packet);
			buffer_t buf = packet_buf(p->_packet);
			char *ptr = (char*)&buf->buf[begpos];
			lua_pushlstring(L,(const char*)ptr,(size_t)packet_datasize(p->_packet));
		}
		else
			lua_pushnil(L);
		return 1;
	}else
		return luaL_error(L,"invaild opration");		
}

static int _peek_uint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,rpk_peek_uint8(rpk));
	return 1;	
}

static int _peek_uint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		lua_pushnil(L);	
	else{
		rpacket_t rpk = (rpacket_t)p->_packet;
		lua_pushinteger(L,rpk_read_uint16(rpk));
	}
	return 1;	
}

static int _to_string(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type == RPACKET)
		lua_pushstring(L,"RPACKET");
	else if(p->_packet->type == WPACKET)
		lua_pushstring(L,"WPACKET");
	else if(p->_packet->type == RAWPACKET)
		lua_pushstring(L,"RAWPACKET");
	else if(p->_packet->type == HTTPPACKET){
		const char *ev_type = ((httppacket_t)p->_packet)->ev_type;
		lua_pushstring(L,ev_type);
	}
	else
		lua_pushnil(L);	
	return 1;
}

static int _peek_uint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		lua_pushnil(L);	
	else{
		rpacket_t rpk = (rpacket_t)p->_packet;
		lua_pushinteger(L,rpk_peek_uint32(rpk));
	}
	return 1;	
}


static int _peek_double(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		lua_pushnil(L);	
	else{
		rpacket_t rpk = (rpacket_t)p->_packet;
		lua_pushnumber(L,rpk_peek_double(rpk));
	}
	return 1;	
}


static int lua_new_wpacket(lua_State *L){
	return lua_new_packet(L,WPACKET);
}

static int lua_new_rpacket(lua_State *L){
	return lua_new_packet(L,RPACKET);
}

static int lua_new_rawpacket(lua_State *L){
	return lua_new_packet(L,RAWPACKET);
}


void reg_luapacket(lua_State *L){
    luaL_Reg packet_mt[] = {
        {"__gc", destroy_luapacket},
        {NULL, NULL}
    };

    luaL_Reg packet_methods[] = {
        {"Read_uint8", _read_uint8},
        {"Read_uint16", _read_uint16},
        {"Read_uint32", _read_uint32},
        {"Read_double", _read_double},        
        {"Read_string", _read_string},
        {"Read_rawbin", _read_rawbin},
        {"Peek_uint8", _peek_uint8},
        {"Peek_uint16", _peek_uint16},
        {"Peek_uint32", _peek_uint32},
        {"Peek_double", _peek_double},                     
        
        {"Write_uint8", _write_uint8},
        {"Write_uint16", _write_uint16},
        {"Write_uint32", _write_uint32},
        {"Write_double",_write_double},        
        {"Write_string",_write_string},
        {"ToString",_to_string},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAPACKET_METATABLE);
    luaL_setfuncs(L, packet_mt, 0);

    luaL_newlib(L, packet_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_Reg l[] = {
        {"NewWPacket",lua_new_wpacket},
        {"NewRPacket",lua_new_rpacket},
        {"NewRawPacket",lua_new_rawpacket},                    
        {NULL, NULL}
    };
    luaL_newlib(L, l);
	lua_setglobal(L,"CPacket");    	
}

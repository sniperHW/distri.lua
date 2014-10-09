#include "luapacket.h"


static int lua_new_packet(lua_State *L,int packettype){
	int argtype = lua_type(L,1); 
	if(packettype == WPACKET){
		if(argtype == LUA_TNUMBER){
			//参数为数字,构造一个初始大小为len的wpacket
			size_t len = size_of_pow2(lua_tointeger(L,1));
			if(len < 64) len = 64;
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)wpk_create(len);
			return 1;			
		}else if(argtype == LUA_TSTRING){
			size_t len;
			char *data = (char*)lua_tolstring(L,1,&len);
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)wpk_create_by_bin((int8_t*)data,len);
			return 1;				
		}else if(argtype ==  LUA_TUSERDATA){
			lua_packet_t other = lua_getluapacket(L,1);
			if(!other)
				return luaL_error(L,"invaild opration for arg1");
			if(other->_packet->type == RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)wpk_copy_create(other->_packet);
			return 1;												
		}else
			return luaL_error(L,"invaild opration for arg1");		
	}else if(packettype == RAWPACKET){
		if(argtype == LUA_TSTRING){
			//参数为string,构造一个函数数据data的rawpacket
			size_t len;
			char *data = (char*)lua_tolstring(L,1,&len);
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);				
			p->_packet = (packet_t)rawpacket_create2(data,len);
			return 1;
		}else if(argtype ==  LUA_TUSERDATA){
			lua_packet_t other = lua_getluapacket(L,1);
			if(!other)
				return luaL_error(L,"invaild opration for arg1");
			if(other->_packet->type != RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)rawpacket_copy_create((rawpacket_t)other->_packet);
			return 1;							
		}else
			return luaL_error(L,"invaild opration for arg1");
	}else if(packettype == RPACKET){
		if(argtype ==  LUA_TUSERDATA){
			lua_packet_t other = lua_getluapacket(L,1);
			if(!other)
				return luaL_error(L,"invaild opration for arg1");
			if(other->_packet->type == RAWPACKET)
				return luaL_error(L,"invaild opration for arg1");
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)rpk_copy_create(other->_packet);
			return 1;					
		}else
			return luaL_error(L,"invaild opration for arg1");	
	}else
		return luaL_error(L,"invaild packet type");
}

/*
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
}*/

void push_luapacket(lua_State *L,packet_t pk){
	lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(lua_packet_t));
	luaL_getmetatable(L, LUAPACKET_METATABLE);
	lua_setmetatable(L, -2);
	p->_packet = pk;
}


static int destroy_luapacket(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet){ 
		destroy_packet(p->_packet);
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

static int _write_wpk(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	lua_packet_t v = lua_getluapacket(L,2);
	if(!v->_packet || v->_packet->type != WPACKET)
		return luaL_error(L,"invaild arg2");
	wpk_write_wpk((wpacket_t)p->_packet,(wpacket_t)v->_packet);	
	return 0;			
}

static int _rewrite_uint8(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");	
	write_pos wpos;
	lua_rawgeti(L,2,1);
	wpos.buf = lua_touserdata(L,-1);
	lua_pop(L,1);
	lua_rawgeti(L,2,2);
	wpos.wpos = (uint32_t)lua_tointeger(L,-1);
	lua_pop(L,1);
	wpk_rewrite_uint8(&wpos,(uint8_t)lua_tointeger(L,3));	
	return 0;	
}

static int _rewrite_uint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");	
	write_pos wpos;
	lua_rawgeti(L,2,1);
	wpos.buf = lua_touserdata(L,-1);
	lua_pop(L,1);
	lua_rawgeti(L,2,2);
	wpos.wpos = (uint32_t)lua_tointeger(L,-1);
	lua_pop(L,1);
	wpk_rewrite_uint16(&wpos,(uint16_t)lua_tointeger(L,3));	
	return 0;	
}

static int _rewrite_uint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");	
	write_pos wpos;
	lua_rawgeti(L,2,1);
	wpos.buf = lua_touserdata(L,-1);
	lua_pop(L,1);
	lua_rawgeti(L,2,2);
	wpos.wpos = (uint32_t)lua_tointeger(L,-1);
	lua_pop(L,1);
	wpk_rewrite_uint32(&wpos,(uint32_t)lua_tointeger(L,3));	
	return 0;	
}

static int _rewrite_double(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");
	if(lua_type(L,2) != LUA_TTABLE)
		return luaL_error(L,"invaild arg2");
	if(lua_type(L,3) != LUA_TNUMBER)
		return luaL_error(L,"invaild arg3");	
	write_pos wpos;
	lua_rawgeti(L,2,1);
	wpos.buf = lua_touserdata(L,-1);
	lua_pop(L,1);
	lua_rawgeti(L,2,2);
	wpos.wpos = (uint32_t)lua_tointeger(L,-1);
	lua_pop(L,1);
	wpk_rewrite_double(&wpos,(double)lua_tonumber(L,3));		
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
	if(!p->_packet || p->_packet->type != RAWPACKET)
		return luaL_error(L,"invaild opration");
		
	rawpacket_t rawpk = (rawpacket_t)p->_packet;
	uint32_t len = 0;
	const char *data = rawpacket_data(rawpk,&len);
	lua_pushlstring(L,data,(size_t)len);
	return 1;						
}

static int _reset_rpos(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	reset_rpos(rpk);
	return 0;	
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

static int _get_write_pos(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");		
	write_pos wpos = wpk_get_writepos((wpacket_t)p->_packet);
	lua_newtable(L);
	lua_pushlightuserdata(L,wpos.buf);
	lua_rawseti(L,-2,1);
	lua_pushinteger(L,wpos.wpos);
	lua_rawseti(L,-2,2);
	return 1;		
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
        {"Reset_rpos",  _reset_rpos},                     
        
        {"Write_uint8", _write_uint8},
        {"Write_uint16", _write_uint16},
        {"Write_uint32", _write_uint32},
        {"Write_double",_write_double},        
        {"Write_string",_write_string},
        {"Write_wpk",   _write_wpk},

        {"Rewrite_uint8",  _rewrite_uint8},
        {"Rewrite_uint16", _rewrite_uint16},
        {"Rewrite_uint32", _rewrite_uint32},
        {"Rewrite_double", _rewrite_double},        
     
		{"Get_write_pos", _get_write_pos},
		
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

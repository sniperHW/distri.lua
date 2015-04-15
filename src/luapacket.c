#include "luapacket.h"

enum{
	L_TABLE = 1,
	L_STRING,
	L_BOOL,
	L_ARRAY,
	L_FLOAT,
	L_UINT8,
	L_UINT16,
	L_UINT32,
	L_UINT64,
	L_INT8,
	L_INT16,
	L_INT32,
	L_INT64,		
};


#define VAILD_KEY_TYPE(TYPE) (TYPE == LUA_TSTRING || TYPE == LUA_TNUMBER)
#define VAILD_VAILD_TYPE(TYPE) (TYPE == LUA_TSTRING || TYPE == LUA_TNUMBER || TYPE == LUA_TTABLE || TYPE == LUA_TBOOLEAN)


static inline void luabin_pack_string(wpacket_t wpk,lua_State *L,int index){
	wpk_write_uint8(wpk,L_STRING);
	size_t len;
	const char *data = lua_tolstring(L,index,&len);
	wpk_write_binary(wpk,data,len);	
}

static inline void luabin_pack_number(wpacket_t wpk,lua_State *L,int index){
	lua_Number v = lua_tonumber(L,index);
	if(v != (lua_Integer)v){
		wpk_write_uint8(wpk,L_FLOAT);
		wpk_write_double(wpk,v);
	}else{
		if((int64_t)v > 0){
			uint64_t _v = (uint64_t)v;
			if(_v <= 0xFF){
				wpk_write_uint8(wpk,L_UINT8);
				wpk_write_uint8(wpk,(uint8_t)_v);				
			}else if(_v <= 0xFFFF){
				wpk_write_uint8(wpk,L_UINT16);
				wpk_write_uint16(wpk,(uint16_t)_v);					
			}else if(_v <= 0xFFFFFFFF){
				wpk_write_uint8(wpk,L_UINT32);
				wpk_write_uint32(wpk,(uint32_t)_v);					
			}else{
				wpk_write_uint8(wpk,L_UINT64);
				wpk_write_uint64(wpk,(uint64_t)_v);				
			}
		}else{
			int64_t _v = (int64_t)v;
			if(_v >= 0x80){
				wpk_write_uint8(wpk,L_INT8);
				wpk_write_uint8(wpk,(uint8_t)_v);				
			}else if(_v >= 0x8000){
				wpk_write_uint8(wpk,L_INT16);
				wpk_write_uint16(wpk,(uint16_t)_v);					
			}else if(_v < 0x80000000){
				wpk_write_uint8(wpk,L_INT32);
				wpk_write_uint32(wpk,(uint32_t)_v);					
			}else{
				wpk_write_uint8(wpk,L_INT64);
				wpk_write_uint64(wpk,(uint64_t)_v);				
			}
		}
	}
}

static inline void luabin_pack_boolean(wpacket_t wpk,lua_State *L,int index){
	wpk_write_uint8(wpk,L_BOOL);
	int value = lua_toboolean(L,index);
	wpk_write_uint8(wpk,value);
}

static int luabin_pack_table(wpacket_t wpk,lua_State *L,int index);

static int luabin_pack_array(wpacket_t wpk,lua_State *L,int index,uint32_t size){
	wpk_write_uint8(wpk,L_ARRAY);
	write_pos wpos = wpk_get_writepos(wpk);
	wpk_write_uint32(wpk,0);
	int ret = 0;
	int c = 0;
	int top = lua_gettop(L);
	lua_pushnil(L);
	do{		
		if(!lua_next(L,index - 1)){
			break;
		}
		int val_type = lua_type(L, -1);
		if(!VAILD_VAILD_TYPE(val_type)){
			lua_pop(L,1);
			continue;
		}

		if(val_type == LUA_TSTRING)
			luabin_pack_string(wpk,L,-1);
		else if(val_type == LUA_TNUMBER)
			luabin_pack_number(wpk,L,-1);
		else if(val_type == LUA_TBOOLEAN)
			luabin_pack_boolean(wpk,L,-1);
		else{
			uint32_t size = lua_rawlen(L,-1);
			if(size > 0){
				if(0 != (ret = luabin_pack_array(wpk,L,-1,size)))
					break;				
			}else{
				if(0 != (ret = luabin_pack_table(wpk,L,-1)))
					break;
			}
		}
		lua_pop(L,1);
		++c;
	}while(1);
	lua_settop(L,top);
	if(0 == ret){
		wpk_rewrite_uint32(&wpos,c);
	}						
	return ret;
}

static int luabin_pack_table(wpacket_t wpk,lua_State *L,int index){
	if(0 != lua_getmetatable(L,index)){
		lua_pop(L,1);
		return -1;
	}
	wpk_write_uint8(wpk,L_TABLE);
	write_pos wpos = wpk_get_writepos(wpk);
	wpk_write_uint32(wpk,0);
	int ret = 0;
	int c = 0;
	int top = lua_gettop(L);
	lua_pushnil(L);
	do{		
		if(!lua_next(L,index - 1)){
			break;
		}
		int key_type = lua_type(L, -2);
		int val_type = lua_type(L, -1);
		if(!VAILD_KEY_TYPE(key_type)){
			lua_pop(L,1);
			continue;
		}
		if(!VAILD_VAILD_TYPE(val_type)){
			lua_pop(L,1);
			continue;
		}
		if(key_type == LUA_TSTRING)
			luabin_pack_string(wpk,L,-2);
		else
			luabin_pack_number(wpk,L,-2);

		if(val_type == LUA_TSTRING)
			luabin_pack_string(wpk,L,-1);
		else if(val_type == LUA_TNUMBER)
			luabin_pack_number(wpk,L,-1);
		else if(val_type == LUA_TBOOLEAN)
			luabin_pack_boolean(wpk,L,-1);
		else{
			uint32_t size = lua_rawlen(L,-1);
			if(size > 0){
				if(0 != (ret = luabin_pack_array(wpk,L,-1,size)))
					break;				
			}else{
				if(0 != (ret = luabin_pack_table(wpk,L,-1)))
					break;
			}
		}
		lua_pop(L,1);
		++c;
	}while(1);
	lua_settop(L,top);
	if(0 == ret){
		wpk_rewrite_uint32(&wpos,c);
	}						
	return ret;
}

static int lua_new_packet(lua_State *L,int packettype){
	int argtype = lua_type(L,1); 
	if(packettype == WPACKET){
		if(argtype == LUA_TNUMBER || argtype == LUA_TNIL || argtype == LUA_TNONE){
			//参数为数字,构造一个初始大小为len的wpacket
			size_t len = 0;
			if(argtype == LUA_TNUMBER) len = size_of_pow2(lua_tointeger(L,1));
			if(len < 64) len = 64;
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = (packet_t)wpk_create(len);
			return 1;			
		}else if(argtype == LUA_TSTRING){
			size_t len;
			char *data = (char*)lua_tolstring(L,1,&len);
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
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
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = make_writepacket(other->_packet);//(packet_t)wpk_copy_create(other->_packet);
			return 1;												
		}else if(argtype == LUA_TTABLE){
			wpacket_t wpk = wpk_create(512);
			if(0 != luabin_pack_table(wpk,L,-1)){
				destroy_packet((packet_t)wpk);
				return luaL_error(L,"table should not hava metatable");	
				//lua_pushnil(L);
			}else{
				lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
				luaL_getmetatable(L, LUAPACKET_METATABLE);
				lua_setmetatable(L, -2);
				p->_packet = (packet_t)wpk;		
			}
			return 1;
		}
		else	
			return luaL_error(L,"invaild opration for arg1");		
	}else if(packettype == RAWPACKET){
		if(argtype == LUA_TSTRING){
			//参数为string,构造一个函数数据data的rawpacket
			size_t len;
			char *data = (char*)lua_tolstring(L,1,&len);
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
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
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = clone_packet(other->_packet);
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
			lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
			luaL_getmetatable(L, LUAPACKET_METATABLE);
			lua_setmetatable(L, -2);
			p->_packet = make_readpacket(other->_packet);
			return 1;					
		}else
			return luaL_error(L,"invaild opration for arg1");	
	}else
		return luaL_error(L,"invaild packet type");
}

void push_luapacket(lua_State *L,packet_t pk){
	lua_packet_t p = (lua_packet_t)lua_newuserdata(L, sizeof(*p));
	luaL_getmetatable(L, LUAPACKET_METATABLE);
	lua_setmetatable(L, -2);
	p->_packet = pk;
}


static int destroy_luapacket(lua_State *L) {
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet){ 
		destroy_packet(p->_packet);
		p->_packet = NULL;
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


int _write_table(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != WPACKET)
		return luaL_error(L,"invaild opration");	
	if(LUA_TTABLE != lua_type(L, 2))
		return luaL_error(L,"argument should be lua table");
	if(0 != luabin_pack_table((wpacket_t)p->_packet,L,-1))
		return luaL_error(L,"table should not hava metatable");	
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
	if(data)
		lua_pushlstring(L,data,(size_t)len);
	else
		lua_pushnil(L);
	return 1;
}

static inline void un_pack_boolean(rpacket_t rpk,lua_State *L){
	int n = rpk_read_uint8(rpk);
	lua_pushboolean(L,n);
}

static inline void un_pack_number(rpacket_t rpk,lua_State *L,int type){
	double n;// = rpk_read_double(rpk);
	switch(type){
		case L_FLOAT:{
			n = rpk_read_double(rpk);
			break;
		}
		case L_UINT8:{
			n = (double)rpk_read_uint8(rpk);
			break;
		}
		case L_UINT16:{
			n = (double)rpk_read_uint16(rpk);
			break;
		}
		case L_UINT32:{
			n = (double)rpk_read_uint32(rpk);
			break;
		}
		case L_UINT64:{
			n = (double)rpk_read_uint64(rpk);
			break;
		}
		case L_INT8:{
			n = (double)((int8_t)rpk_read_uint8(rpk));
			break;
		}
		case L_INT16:{
			n = (double)((int16_t)rpk_read_uint16(rpk));
			break;
		}
		case L_INT32:{
			n = (double)((int32_t)rpk_read_uint32(rpk));
			break;
		}
		case L_INT64:{
			n = (double)((int64_t)rpk_read_uint64(rpk));
			break;
		}
		default:{
			assert(0);
			break;						
		}
	}
	lua_pushnumber(L,n);
}

static inline void un_pack_string(rpacket_t rpk,lua_State *L){
	uint32_t len;
	const char *data = rpk_read_binary(rpk,&len);
	lua_pushlstring(L,data,(size_t)len);
}
static int un_pack_table(rpacket_t rpk,lua_State *L);
static int un_pack_array(rpacket_t rpk,lua_State *L){

	int size = rpk_read_uint32(rpk);
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int value_type = rpk_read_uint8(rpk);
		if(value_type == L_STRING){
			un_pack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			un_pack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			un_pack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			if(0 != un_pack_table(rpk,L)){
				return -1;
			}
		}else if(value_type == L_ARRAY){
			if(0 != un_pack_array(rpk,L)){
				return -1;
			}
		}else
			return -1;
		lua_rawseti(L,-2,i+1);		
	}
	return 0;	
}

static int un_pack_table(rpacket_t rpk,lua_State *L){
	int size = rpk_read_uint32(rpk);
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int key_type,value_type;
		key_type = rpk_read_uint8(rpk);
		if(key_type == L_STRING){
			un_pack_string(rpk,L);
		}else if(key_type >= L_FLOAT && key_type <= L_INT64){
			un_pack_number(rpk,L,key_type);
		}else
			return -1;
		value_type = rpk_read_uint8(rpk);
		if(value_type == L_STRING){
			un_pack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			un_pack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			un_pack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			if(0 != un_pack_table(rpk,L)){
				return -1;
			}
		}else if(value_type == L_ARRAY){
			if(0 != un_pack_array(rpk,L)){
				return -1;
			}
		}else
			return -1;
		lua_rawset(L,-3);			
	}
	return 0;
}

static int to_lua_table(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = 	(rpacket_t)p->_packet;

	//backup
	uint32_t rpos = rpk->rpos;
	uint32_t data_remain = packet_dataremain(rpk);
	buffer_t readbuf = rpk->readbuf;

	do{
		int type = rpk_read_uint8(rpk);
		if(type != L_TABLE){
			lua_pushnil(L);
			break;
		}	
		int top = lua_gettop(L);
		int ret = un_pack_table(rpk,L);
		if(0 != ret){
			lua_settop(L,top);
			lua_pushnil(L);
		}		
	}while(0);

	//recover
	rpk->rpos = rpos;
	packet_dataremain(rpk) = data_remain; 
	rpk->readbuf = readbuf;
	return 1;
}

static int _read_table(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(!p->_packet || p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = 	(rpacket_t)p->_packet;
	int type = rpk_read_uint8(rpk);
	if(type != L_TABLE){
		lua_pushnil(L);
		return 1;
	}	
	int old_top = lua_gettop(L);
	int ret = un_pack_table(rpk,L);
	if(0 != ret){
		lua_settop(L,old_top);
		lua_pushnil(L);
	}
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

static int _reverse_read_uint16(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,reverse_read_uint16(rpk));
	return 1;	
}

static int _reverse_read_uint32(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	lua_pushinteger(L,reverse_read_uint32(rpk));
	return 1;		
}

/*static int _reset_rpos(lua_State *L){
	lua_packet_t p = lua_getluapacket(L,1);
	if(p->_packet->type != RPACKET)
		return luaL_error(L,"invaild opration");
	rpacket_t rpk = (rpacket_t)p->_packet;
	reset_rpos(rpk);
	return 0;	
}*/

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
		lua_pushinteger(L,rpk_peek_uint16(rpk));
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
        {"Read_table", _read_table},
        {"Read_rawbin", _read_rawbin},
        {"Reverse_read_uint16", _reverse_read_uint16},
        {"Reverse_read_uint32", _reverse_read_uint32},
        
        
        
        {"Peek_uint8", _peek_uint8},
        {"Peek_uint16", _peek_uint16},
        {"Peek_uint32", _peek_uint32},
        {"Peek_double", _peek_double},
        //{"Reset_rpos",  _reset_rpos},                     
        
        {"Write_uint8", _write_uint8},
        {"Write_uint16", _write_uint16},
        {"Write_uint32", _write_uint32},
        {"Write_double",_write_double},        
        {"Write_string", _write_string},
        {"Write_wpk",   _write_wpk},
        {"Write_table",  _write_table},

        {"Rewrite_uint8",  _rewrite_uint8},
        {"Rewrite_uint16", _rewrite_uint16},
        {"Rewrite_uint32", _rewrite_uint32},
        {"Rewrite_double", _rewrite_double},        
        {"Get_write_pos", _get_write_pos},
        {"ToTable",to_lua_table},
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

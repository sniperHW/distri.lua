#include "lua/lua_util.h"
#include "lua/lua_util_packet.h"

enum{
	L_TABLE = 1,
	L_STRING,
	L_BOOL,
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


static inline void _lua_pack_string(wpacket_t wpk,lua_State *L,int index){
	wpk_write_uint8(wpk,L_STRING);
	size_t len;
	const char *data = lua_tolstring(L,index,&len);
	wpk_write_binary(wpk,data,len);	
}

static  void _lua_pack_number(wpacket_t wpk,lua_State *L,int index){
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

static inline void _lua_pack_boolean(wpacket_t wpk,lua_State *L,int index){
	wpk_write_uint8(wpk,L_BOOL);
	wpk_write_uint8(wpk,lua_toboolean(L,index));
}


enum {
	ERR_NOT_TABLE,
	ERR_UNSUPPORT_TYPE,
	ERR_METATABLE,
};

const char *lua_pack_error_msg[] = {
	"param should be a lua table",
	"lua table contain unsupport value type",
	"lua table contain metatable",
};

static const char * _lua_pack_table(wpacket_t wpk,lua_State *L,int index){
	wpk_write_uint8(wpk,L_TABLE);
	write_pos wpos = wpk_get_writepos(wpk);
	wpk_write_uint32(wpk,0);
	int c = 0;
	int top = lua_gettop(L);
	lua_pushnil(L);
	do{		
		if(!lua_next(L,index - 1)){
			break;
		}
		int key_type = lua_type(L, -2);
		int val_type = lua_type(L, -1);
		if(VAILD_KEY_TYPE(key_type) && VAILD_VAILD_TYPE(val_type)){
			if(key_type == LUA_TSTRING)
				_lua_pack_string(wpk,L,-2);
			else
				_lua_pack_number(wpk,L,-2);

			if(val_type == LUA_TSTRING)
				_lua_pack_string(wpk,L,-1);
			else if(val_type == LUA_TNUMBER)
				_lua_pack_number(wpk,L,-1);
			else if(val_type == LUA_TBOOLEAN)
				_lua_pack_boolean(wpk,L,-1);
			else if(val_type == LUA_TTABLE){
				if(0 != lua_getmetatable(L,index)){ 
					return lua_pack_error_msg[ERR_METATABLE];
				}
				const char *errmsg;
				if((errmsg = _lua_pack_table(wpk,L,-1)))
					return errmsg;
			}else{
				return lua_pack_error_msg[ERR_UNSUPPORT_TYPE];
			}
			++c;
		}
		lua_pop(L,1);
	}while(1);
	lua_settop(L,top);
	wpk_rewrite_uint32(&wpos,c);						
	return NULL;
}


const char *lua_pack_table(wpacket_t wpk,lua_State *L,int index){
	if(lua_type(L, index) != LUA_TTABLE)
		return lua_pack_error_msg[ERR_NOT_TABLE];
	if(0 != lua_getmetatable(L,index))
		return lua_pack_error_msg[ERR_METATABLE];
	return _lua_pack_table(wpk,L,-1);	
}


static inline int  _lua_unpack_boolean(rpacket_t rpk,lua_State *L){
	lua_pushboolean(L,rpk_read_uint8(rpk));
	return 0;
}

static inline int _lua_unpack_number(rpacket_t rpk,lua_State *L,int type){
	lua_Integer   n;
	switch(type){
		case L_FLOAT:{
			lua_pushnumber(L, rpk_read_double(rpk));
			return 0;
		}
		case L_UINT8:{
			n = rpk_read_uint8(rpk);
			break;
		}
		case L_UINT16:{
			n = rpk_read_uint16(rpk);
			break;
		}
		case L_UINT32:{
			n = rpk_read_uint32(rpk);
			break;
		}
		case L_UINT64:{
			n = rpk_read_uint64(rpk);
			break;
		}
		case L_INT8:{
			n = ((int8_t)rpk_read_uint8(rpk));
			break;
		}
		case L_INT16:{
			n = ((int16_t)rpk_read_uint16(rpk));
			break;
		}
		case L_INT32:{
			n = ((int32_t)rpk_read_uint32(rpk));
			break;
		}
		case L_INT64:{
			n = ((int64_t)rpk_read_uint64(rpk));		
			break;
		}
		default:{
			assert(0);
			return -1;						
		}
	}
	lua_pushinteger(L,n);
	return 0;
}

static inline int _lua_unpack_string(rpacket_t rpk,lua_State *L){
	uint32_t len = 0;
	const char *data = rpk_read_binary(rpk,&len);
	if(!data) return -1;
	lua_pushlstring(L,data,(size_t)len);
	return 0;
}

static int _lua_unpack_table(rpacket_t rpk,lua_State *L){
	int size = rpk_read_uint32(rpk);
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int key_type,value_type;
		int ret = -1;
		key_type = rpk_read_uint8(rpk);
		if(key_type == L_STRING){
			ret = _lua_unpack_string(rpk,L);
		}else if(key_type >= L_FLOAT && key_type <= L_INT64){
			ret = _lua_unpack_number(rpk,L,key_type);
		}
		if(ret != 0) return -1;
		ret = -1;
		value_type = rpk_read_uint8(rpk);
		if(value_type == L_STRING){
			ret = _lua_unpack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			ret = _lua_unpack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			ret = _lua_unpack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			ret = _lua_unpack_table(rpk,L);
		}
		if(ret != 0) return -1;
		lua_rawset(L,-3);			
	}
	return 0;
}

int lua_unpack_table(rpacket_t rpk,lua_State *L){
	if(rpk_read_uint8(rpk) == L_TABLE && 0 == _lua_unpack_table(rpk,L))
		return 0;
	return -1;
}

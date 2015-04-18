#include "lua/lua_util.h"
#include "lua/lua_util_packet.h"

enum{
	L_TABLE = 1,
	L_STRING,
	L_BOOL,
//	L_ARRAY,
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
	int value = lua_toboolean(L,index);
	wpk_write_uint8(wpk,value);
}

/*static int _lua_pack_table(wpacket_t wpk,lua_State *L,int index);

static int _lua_pack_array(wpacket_t wpk,lua_State *L,int index,uint32_t size){
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
			_lua_pack_string(wpk,L,-1);
		else if(val_type == LUA_TNUMBER)
			_lua_pack_number(wpk,L,-1);
		else if(val_type == LUA_TBOOLEAN)
			_lua_pack_boolean(wpk,L,-1);
		else if(val_type == LUA_TTABLE){
			uint32_t size = lua_rawlen(L,-1);
			if(size > 0){
				if(0 != (ret = _lua_pack_array(wpk,L,-1,size)))
					break;				
			}else{
				if(0 != (ret = _lua_pack_table(wpk,L,-1)))
					break;
			}
		}else{
			ret = -1;
		}
		lua_pop(L,1);
		++c;
	}while(1);
	lua_settop(L,top);
	if(0 == ret){
		wpk_rewrite_uint32(&wpos,c);
	}						
	return ret;
}*/

static int _lua_pack_table(wpacket_t wpk,lua_State *L,int index){
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
			//uint32_t size = lua_rawlen(L,-1);
			//if(size > 0){
			//	if(0 != (ret = _lua_pack_array(wpk,L,-1,size)))
			//		break;				
			//}else{
				if(0 != (ret = _lua_pack_table(wpk,L,-1)))
					break;
			//}
		}else{
			ret = -1;
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


int lua_pack_table(wpacket_t wpk,lua_State *L,int index){
	int ret = -1;
	do{	
		if(lua_type(L, index) != LUA_TTABLE || 0 != lua_getmetatable(L,index)){
			break;
		}
		/*uint32_t size = lua_rawlen(L,-1);
		if(size > 0){
			if(0 != (ret = _lua_pack_array(wpk,L,-1,size)))
				break;				
		}else{*/
			if(0 != (ret = _lua_pack_table(wpk,L,-1)))
				break;
		//}
		ret = 0;
	}while(0);
	return ret;	
}


static inline void _lua_unpack_boolean(rpacket_t rpk,lua_State *L){
	int n = rpk_read_uint8(rpk);
	lua_pushboolean(L,n);
}

static inline void _lua_unpack_number(rpacket_t rpk,lua_State *L,int type){
	lua_Number n1;
	lua_Integer   n2;
	switch(type){
		case L_FLOAT:{
			n1 = rpk_read_double(rpk);
			lua_pushnumber(L,n1);
			break;
		}
		case L_UINT8:{
			n2 = (double)rpk_read_uint8(rpk);
			lua_pushinteger(L,n2);
			break;
		}
		case L_UINT16:{
			n2 = (double)rpk_read_uint16(rpk);
			lua_pushinteger(L,n2);
			break;
		}
		case L_UINT32:{
			n2 = (double)rpk_read_uint32(rpk);
			lua_pushinteger(L,n2);
			break;
		}
		case L_UINT64:{
			n2 = (double)rpk_read_uint64(rpk);
			lua_pushinteger(L,n2);
			break;
		}
		case L_INT8:{
			n2 = (double)((int8_t)rpk_read_uint8(rpk));
			lua_pushinteger(L,n2);
			break;
		}
		case L_INT16:{
			n2 = (double)((int16_t)rpk_read_uint16(rpk));
			lua_pushinteger(L,n2);
			break;
		}
		case L_INT32:{
			n2 = (double)((int32_t)rpk_read_uint32(rpk));
			lua_pushinteger(L,n2);
			break;
		}
		case L_INT64:{
			n2 = (double)((int64_t)rpk_read_uint64(rpk));
			lua_pushinteger(L,n2);
			break;
		}
		default:{
			assert(0);
			break;						
		}
	}
}

static inline void _lua_unpack_string(rpacket_t rpk,lua_State *L){
	uint32_t len;
	const char *data = rpk_read_binary(rpk,&len);
	lua_pushlstring(L,data,(size_t)len);
}

/*static int _lua_unpack_table(rpacket_t rpk,lua_State *L);

int _lua_unpack_array(rpacket_t rpk,lua_State *L){
	int size = rpk_read_uint32(rpk);
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int value_type = rpk_read_uint8(rpk);
		if(value_type == L_STRING){
			_lua_unpack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			_lua_unpack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			_lua_unpack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			if(0 != _lua_unpack_table(rpk,L)){
				return -1;
			}
		}else if(value_type == L_ARRAY){
			if(0 != _lua_unpack_array(rpk,L)){
				return -1;
			}
		}else
			return -1;
		lua_rawseti(L,-2,i+1);		
	}
	return 0;	
}*/

static int _lua_unpack_table(rpacket_t rpk,lua_State *L){
	int size = rpk_read_uint32(rpk);
	int i = 0;
	lua_newtable(L);
	for(; i < size; ++i){
		int key_type,value_type;
		key_type = rpk_read_uint8(rpk);
		if(key_type == L_STRING){
			_lua_unpack_string(rpk,L);
		}else if(key_type >= L_FLOAT && key_type <= L_INT64){
			_lua_unpack_number(rpk,L,key_type);
		}else
			return -1;
		value_type = rpk_read_uint8(rpk);
		if(value_type == L_STRING){
			_lua_unpack_string(rpk,L);
		}else if(value_type >= L_FLOAT && value_type <= L_INT64){
			_lua_unpack_number(rpk,L,value_type);
		}else if(value_type == L_BOOL){
			_lua_unpack_boolean(rpk,L);
		}else if(value_type == L_TABLE){
			if(0 != _lua_unpack_table(rpk,L)){
				return -1;
			}
		}/*else if(value_type == L_ARRAY){
			if(0 != _lua_unpack_array(rpk,L)){
				return -1;
			}
		}*/else
			return -1;
		lua_rawset(L,-3);			
	}
	return 0;
}

int lua_unpack_table(rpacket_t rpk,lua_State *L){
	int ret = -1;
	do{
		int type = rpk_read_uint8(rpk);
		
		if(type == L_TABLE){
			if(0 != _lua_unpack_table(rpk,L))
				break;
		}/*else if(type == L_ARRAY){
			if(0 != _lua_unpack_array(rpk,L))
				break;
		}*/else
			break;
		ret = 0;
	}while(0);
	if(ret != 0)
		lua_pushnil(L);
	return ret;
}

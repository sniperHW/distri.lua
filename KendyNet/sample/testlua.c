#include "lua/lua_util.h"
#include "lua/lua_util_packet.h"


static int packet_pack(lua_State *L){
	wpacket_t wpk = lua_touserdata(L,1);
	lua_pack_table(wpk,L,2);
	return 0;
}

static int packet_unpack(lua_State *L){
	rpacket_t rpk = lua_touserdata(L,1);
	lua_unpack_table(rpk,L);
	return 1;
}
		
int main(int argc,char **argv){
	if(argc < 2){
		printf("usage testlua luafile\n");
		return 0;
	}
	lua_State *L = luaL_newstate();
	if (luaL_dofile(L,argv[1])) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	const char *err;
	//dynamic build a lua table and return
	luaRef_t tabRef = makeLuaObjByStr(L,"return {a=100}");
	if(tabRef.L){
		int a = 0;
		err = LuaRef_Get(tabRef,"si","a",&a);
		if(err) printf("%s\n",err);
		else{
			printf("a == %d\n",a);	
		}
		release_luaRef(&tabRef);
	}

	err = LuaCall(L,"fun0",NULL);
	if(err) printf("error on fun0:%s\n",err);
	
	unsigned int iret1;
	err = LuaCall(L,"fun1","is:i",0xfffffffff,"hello",&iret1);
	if(err) printf("error on fun1:%s\n",err);
	else printf("ret1=%u\n",iret1);
	
	char *sret1;
	err = LuaCall(L,"fun2","ss:s","hello","kenny",&sret1);
	if(err) printf("error on fun2:%s\n",err);
	else printf("sret1=%s\n",sret1);
	
	char  *Sret1,*Sret2;
	size_t Lret1,Lret2;
	err = LuaCall(L,"fun3","ssS:SS","1","2","3",sizeof("3"),&Sret1,&Lret1,&Sret2,&Lret2);
	if(err) printf("error on fun3:%s\n",err); 
	else{
		printf("%s %ld\n",Sret1,Lret1);
		printf("%s %ld\n",Sret2,Lret2);		
	}
	
	
	luaRef_t funRef;
	err = LuaCall(L,"fun5",":r",&funRef);
	if(err) printf("error on fun5:%s\n",err);
	LuaCallRefFunc(funRef,NULL);	
	
	
	int num;
	err = LuaCall(L,"fun6",":ri",&tabRef,&num);
	if(err) printf("error on fun6:%s\n",err);
	printf("%d\n",num);
	err = LuaCallTabFuncS(tabRef,"hello",NULL);
	if(err) printf("%s\n",err);
		
	err = LuaRef_Set(tabRef,"siss","f1",99,"f2","hello haha");
	if(err) printf("%s\n",err);
	else{
		int f1;
		const char *f2;
		err = LuaRef_Get(tabRef,"siss","f1",&f1,"f2",&f2);
		if(err) printf("%s\n",err);
		else printf("get %d,%s\n",f1,f2);
	}

	err = LuaRef_Set(tabRef,"sp","f1",NULL);
	if(err) printf("%s\n",err);
	else{
		err = LuaCall(L,"fun7",NULL);
		if(err) printf("error on fun7:%s\n",err);		
	}
	
	lua_getglobal(L,"ttab4");
	tabRef = toluaRef(L,-1);
	LuaTabForEach(tabRef){
		const char *key = lua_tostring(L,EnumKey);
		const char *val = lua_tostring(L,EnumVal);
		printf("%s,%s\n",key,val);
	}

	printf("here write/read table to/from packet in lua\n");
	luaL_openlibs(L);
    	luaL_Reg l[] = {
        		{"packet_pack",packet_pack},
        		{"packet_unpack",packet_unpack},    		              		                   
        		{NULL, NULL}
    	};
    	luaL_newlib(L, l);
	lua_setglobal(L,"C");	
	wpacket_t wpk = wpk_create(128);
	err = LuaCall(L,"fun8","p",wpk);
	if(err){
		printf("error on fun8:%s\n",err);
	}else{
		rpacket_t rpk = (rpacket_t)make_readpacket((packet_t)wpk);
		err = LuaCall(L,"fun9","p",rpk);
		if(err){
			printf("error on fun9:%s\n",err);
		}
	}
	printf("end\n");	
	return 0;	
}

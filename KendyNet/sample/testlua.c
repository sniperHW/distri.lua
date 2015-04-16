#include "lua_util.h"

		
int main(int argc,char **argv){
	if(argc < 2){
		printf("usage testlua luafile\n");
		return 0;
	}
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	if (luaL_dofile(L,argv[1])) {
		const char * error = lua_tostring(L, -1);
		lua_pop(L,1);
		printf("%s\n",error);
	}
	const char *err;
	
	err = LuaCall(L,"fun0",NULL);
	if(err) printf("error on fun0:%s\n",err);
	
	unsigned int iret1;
	err = LuaCall(L,"fun1","i:i",0xFFFFFFFF,&iret1);
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
	
	
	luaRef_t tabRef;
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

	return 0;	
}

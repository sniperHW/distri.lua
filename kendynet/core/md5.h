/**
*  $Id: md5.h,v 1.2 2006/03/03 15:04:49 tomas Exp $
*  Cryptographic module for Lua.
*  @author  Roberto Ierusalimschy
*/


#ifndef md5_h
#define md5_h

//#include <lua.h>
#define HASHSIZE 16

void md5(const char *message, long len, char *output);

/*
* added by sniperHW 2014-02-13 huangweilook@21cn.com
*/
#include <stdint.h>
typedef struct kn_md5{
	union{
		struct {
			uint64_t high64;
			uint64_t low64;
		};
		char data[HASHSIZE];
	};
}kn_md5;


static inline kn_md5 calmd5(const char *message, long len){
	kn_md5 ret;
	md5(message,len,ret.data);
	return ret;
} 

static inline const char* calmd5str(kn_md5 _md5,char *output,long outlen){
	if(outlen < 32) return NULL;
	snprintf(output,32,"%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
			 (uint8_t)_md5.data[0],(uint8_t)_md5.data[1],(uint8_t)_md5.data[2],(uint8_t)_md5.data[3],
			 (uint8_t)_md5.data[4],(uint8_t)_md5.data[5],(uint8_t)_md5.data[6],(uint8_t)_md5.data[7],
			 (uint8_t)_md5.data[8],(uint8_t)_md5.data[9],(uint8_t)_md5.data[10],(uint8_t)_md5.data[11],
			 (uint8_t)_md5.data[12],(uint8_t)_md5.data[13],(uint8_t)_md5.data[14],(uint8_t)_md5.data[15]);
	return output;
}

static inline const char* md5str(const char *message,long len,char *output,long outlen){
	return calmd5str(calmd5(message,len),output,outlen);
}

//int luaopen_md5_core (lua_State *L);


#endif
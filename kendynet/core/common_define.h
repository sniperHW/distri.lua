#ifndef _COMMON_DEFINE_H
#define _COMMON_DEFINE_H

//消息类型定义
enum
{
	//逻辑到通信
	MSG_WPACKET = 0,
	MSG_BIND,
	MSG_LISTEN,
	MSG_CONNECT,
	MSG_ACTIVE_CLOSE,
	//通信到逻辑
	MSG_RPACKET,
	MSG_ONCONNECT,
	MSG_ONCONNECTED,
	MSG_DISCONNECTED,
	MSG_CONNECT_FAIL,
	MSG_LISTEN_RET,
	//DB
	MSG_DB_RESULT,
	MSG_END,
	//other
	MSG_DO_FUNCTION,
};


enum
{
	type_asynsock = 1,
	type_msgdisp  = 2,
	type_luaobjcet = 3,
	type_usertype_begin = 4,	
};

#define MAX_UINT32 0xffffffff

#endif

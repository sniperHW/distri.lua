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
	MSG_END,
};

#define MAX_UINT32 0xffffffff

#endif

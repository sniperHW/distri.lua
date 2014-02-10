#ifndef _CMD_H
#define _CMD_H

/*
*   命令码的定义
*/

#define CMD_CLIENT2GATE   100

#define CMD_SUPER2CLIENT   200

#define CMD_GATE2SUPER    300

#define CMD_CLIENT2SUPER  400

#define CMD_CLIENT2BATTLE 500


#define MAX_CMD           1024

enum{
	CMD_INVAILD = 0,
	CMD_PLY_CONNECTING,//用户请求建立连接
	CMD_GAME_BUSY,     //通知玩家系统繁忙
	CMD_PLY_LOGIN = CMD_CLIENT2GATE,
};

struct rpacket;
typedef void (*cmd_handler)(struct rpacket*);

#endif

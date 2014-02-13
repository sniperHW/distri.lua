/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
	MSG_ASYNCALL,
	MSG_ASYNRESULT,
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

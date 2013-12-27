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

/*
# Text File
# Author:   kenny<huangweilook@21cn.com>
# File:     sock_util.h
# Created:  13:11:56 2008-10-27
# Modified: 13:12:06 2008-10-27
# Brief:    一些套接口API的包装函数.
*/

#ifndef _SOCK_WRAPPER_H
#define _SOCK_WRAPPER_H

#include "common.h"

enum sock_family
{
	INET = AF_INET,
	INET6 = AF_INET6,
#ifdef _LINUX
	LOCAL = AF_LOCAL,
	ROUTE = AF_ROUTE,
	KEY = AF_KEY,
#endif
};

enum sock_type
{
	STREAM = SOCK_STREAM,//流协议
	DGRAM  = SOCK_STREAM,//数据报协议
	SEQPACKET = SOCK_SEQPACKET,
	RAW = SOCK_RAW,//原始套接口
};

enum sock_protocol
{
	TCP = IPPROTO_TCP,
	UDP = IPPROTO_UDP,
#ifdef _LINUX
	SCTP = IPPROTO_SCTP,
#endif
};

//从关联的engine中去掉读监听，同时向对端发送读关闭
void    ShutDownRecv(SOCK);

//从关联的engine中去掉写监听,但不向对端发送写关闭
void    ShutDownSend(SOCK);

int32_t CloseSocket(SOCK);

/*
 * brief: 创建套接字,并与给定的对端建立连接.
 * para:
 *        ip:对端的IP地址
 *        port:对端端口
 *        servaddr:
 */
SOCK Tcp_Connect(const char *ip,uint16_t port);


/*
 * brief: 创建套接字,磅定,然后在此套接字上监听.
 *
 */
SOCK Tcp_Listen(const char *ip,uint16_t port,int32_t backlog);

/*
 * brief: 获取远端连接的IP,端口号.
 */
//int32_t getRemoteAddrPort(SOCK sock,char *buf,uint16_t *port);
//int32_t getLocalAddrPort(SOCK sock,struct sockaddr_in *remoAddr,socklen_t *len,char *buf,uint16_t *port);

/*
ssize_t write_fd(int fd,void *ptr,size_t nbytes,int sendfd);

int create_un_execl(const char *path,const char *child);

ssize_t read_fd(int fd,void *ptr,size_t nbytes,int *recvfd);
*/

//struct hostent *Gethostbyaddr(const char *ip,int32_t family);

int32_t setNonblock(SOCK sock);

#endif




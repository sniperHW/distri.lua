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
#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "kendynet.h"
#include "wpacket.h"
#include "rpacket.h"
#include <stdint.h>
#include "timer.h"
#include "allocator.h"
#include "common_define.h"
#include "common.h"
#include "refbase.h"

struct connection;
struct OVERLAPCONTEXT
{
	st_io   m_super;
	struct  connection *c;
};


/*
*   返回1：process_packet调用后rpacket_t自动销毁
*   否则,将由使用者自己销毁
*/
typedef int8_t (*CCB_PROCESS_PKT)(struct connection*,rpacket_t);


typedef void (*CCB_DISCONNECT)(struct connection*,uint32_t reason);
typedef void (*CCB_RECV_TIMEOUT)(struct connection*);
typedef void (*CCB_SEND_TIMEOUT)(struct connection*);

#define MAX_WBAF 512
#define MAX_SEND_SIZE 65536

struct connection
{
	struct refbase ref;
	SOCK socket;
	struct iovec wsendbuf[MAX_WBAF];
	struct iovec wrecvbuf[2];
	struct OVERLAPCONTEXT send_overlap;
	struct OVERLAPCONTEXT recv_overlap;

	uint32_t unpack_size; //还未解包的数据大小
	uint32_t unpack_pos;
	uint32_t next_recv_pos;
	buffer_t next_recv_buf;
	buffer_t unpack_buf;

    struct llist send_list;//待发送的包
    CCB_PROCESS_PKT cb_process_packet;
    CCB_DISCONNECT  cb_disconnect;
	union{
        uint64_t usr_data;
        void    *usr_ptr;
	};
	uint64_t last_recv;
	struct timer_item wheelitem;
	uint32_t recv_timeout;
    uint32_t send_timeout;
    CCB_RECV_TIMEOUT cb_recv_timeout;
    CCB_SEND_TIMEOUT cb_send_timeout;
	uint8_t  raw;
    volatile uint32_t status;
	uint8_t  doing_send;
};


static inline struct timer_item* con2wheelitem(struct connection *con){
    return &con->wheelitem;
}

static inline struct connection *wheelitem2con(struct timer_item *wit)
{
    struct connection *con = (struct connection*)wit;
    return (struct connection *)((char*)wit - ((char*)&con->wheelitem - (char*)con));
}

struct connection *new_conn(SOCK s,uint8_t is_raw);
void   release_conn(struct connection *con);
void   acquire_conn(struct connection *con);

void   active_close(struct connection*);//active close connection

int32_t send_packet(struct connection*,wpacket_t);

int32_t bind2engine(ENGINE,struct connection*,CCB_PROCESS_PKT,CCB_DISCONNECT);

#endif

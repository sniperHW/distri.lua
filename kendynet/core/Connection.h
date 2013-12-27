#ifndef _CONNECTION_H
#define _CONNECTION_H
#include "KendyNet.h"
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
typedef int8_t (*process_packet)(struct connection*,rpacket_t);


typedef void (*on_disconnect)(struct connection*,uint32_t reason);
typedef void (*on_recv_timeout)(struct connection*);
typedef void (*on_send_timeout)(struct connection*);

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

	struct link_list send_list;//待发送的包
	process_packet _process_packet;
	on_disconnect  _on_disconnect;
	union{
        uint64_t usr_data;
        void    *usr_ptr;
	};
	uint32_t last_recv;
	struct timer_item wheelitem;
	uint32_t recv_timeout;
    uint32_t send_timeout;
    on_recv_timeout _recv_timeout;
    on_send_timeout _send_timeout;
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

int32_t bind2engine(ENGINE,struct connection*,process_packet,on_disconnect);

#endif

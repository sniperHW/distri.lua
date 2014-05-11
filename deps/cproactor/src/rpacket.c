#include "rpacket.h"
#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kn_common_define.h"
#include "kn_atomic.h"

kn_allocator_t rpacket_allocator = NULL;

rpacket_t rpk_create(buffer_t b,
                     uint32_t pos,
                     uint32_t pk_len,
                     uint8_t is_raw)
{
	rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
	struct packet *base = (struct packet*)r;
	r->binbuf = NULL;
	r->binbufpos = 0;
    PACKET_BUF(r) = buffer_acquire(NULL,b);
	r->readbuf = buffer_acquire(NULL,b);
	r->len = pk_len;
	r->data_remain = r->len;
    PACKET_BEGINPOS(r) = pos;
    MSG_NEXT(r) = NULL;
    MSG_TYPE(r) = MSG_RPACKET;
	if(is_raw)
		r->rpos = pos;
	else
		r->rpos = (pos + sizeof(r->len))%base->buf->capacity;
	if(r->rpos < base->begin_pos)
		r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);

	base->raw = is_raw;
	return r;
}

static inline rpacket_t rpk_create_by_rpacket(rpacket_t other)
{
	rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
    r->binbuf = NULL;
	r->binbufpos = 0;
    PACKET_BUF(r) = buffer_acquire(NULL,PACKET_BUF(other));
	r->readbuf = buffer_acquire(NULL,other->readbuf);
	r->len = other->len;
    r->data_remain = other->data_remain;
    PACKET_BEGINPOS(r) = PACKET_BEGINPOS(other);
    MSG_NEXT(r) = NULL;
    MSG_TYPE(r) = MSG_RPACKET;
    PACKET_RAW(r) = PACKET_RAW(other);

	r->rpos = other->rpos;
	return r;
}

static inline rpacket_t rpk_create_by_wpacket(struct wpacket *w)
{
	rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
	r->binbuf = NULL;
	r->binbufpos = 0;
    PACKET_BUF(r) = buffer_acquire(NULL,PACKET_BUF(w));
    r->readbuf = buffer_acquire(NULL,PACKET_BUF(w));
    PACKET_BEGINPOS(r) = PACKET_BEGINPOS(w);
    MSG_NEXT(r) = NULL;
    MSG_TYPE(r) = MSG_RPACKET;
    PACKET_RAW(r) = PACKET_RAW(w);
    if(PACKET_RAW(r))
	{
		r->len = w->data_size;
        r->rpos =PACKET_BEGINPOS(w);
	}
	else
	{
		//这里的len只记录构造时wpacket的len,之后wpacket的写入不会影响到rpacket的len
		r->len = w->data_size - sizeof(r->len);
        r->rpos = (PACKET_BEGINPOS(r) + sizeof(r->len))%PACKET_BUF(r)->capacity;
        if(r->rpos < PACKET_BEGINPOS(r))//base->begin_pos)
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
	}
	r->data_remain = r->len;
	return r;
}

rpacket_t rpk_create_by_other(struct packet *p)
{
    if(MSG_TYPE(p) == MSG_RPACKET)
		return rpk_create_by_rpacket((rpacket_t)p);
    else if(MSG_TYPE(p) == MSG_WPACKET)
		return rpk_create_by_wpacket((wpacket_t)p);
	return NULL;
}

void rpk_dropback(rpacket_t r,uint32_t dropsize)
{
    if(!r || PACKET_RAW(r) || r->len < dropsize)
        return;
    r->len -= dropsize;
    if(r->data_remain > dropsize)
        r->data_remain -= dropsize;
    else
        r->data_remain = 0;
    buffer_write(PACKET_BUF(r),PACKET_BEGINPOS(r),(int8_t*)&r->len,sizeof(r->len));
}


rpacket_t rpk_create_skip(rpacket_t other,uint32_t skipsize)
{
    if(!other || PACKET_RAW(other) || other->len <= skipsize)
        return NULL;
    rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
    PACKET_RAW(r) = 0;
    //首先要到正确的读位置
    uint32_t move_size = skipsize+sizeof(r->len);
    buffer_t buf = PACKET_BUF(other);
    uint32_t pos = PACKET_BEGINPOS(other);
    while(move_size){
        uint32_t s = buf->size - pos;
        if(s > move_size) s = move_size;
        move_size -= s;
        pos += s;
        if(pos >= buf->size){
            buf = buf->next;
            pos = 0;
        }
    }

    r->binbuf = NULL;
    r->binbufpos = 0;
    PACKET_BUF(r) = buffer_acquire(NULL,buf);
    r->readbuf = buffer_acquire(NULL,buf);
    r->len = other->len-skipsize;
    r->data_remain = r->len;
    r->rpos = PACKET_BEGINPOS(r) = pos;
    MSG_NEXT(r) = NULL;
    MSG_TYPE(r) = MSG_RPACKET;
    return r;
}

void rpk_destroy(rpacket_t r)
{
	//释放所有对buffer_t的引用
    buffer_release(&PACKET_BUF(r));//(*r)->base.buf);
	buffer_release(&r->readbuf);
	buffer_release(&r->binbuf);
    FREE(rpacket_allocator,r);
}


static const void* rpk_raw_read_binary(rpacket_t r,uint32_t *len)
{
	void *addr = 0;
	uint32_t size = 0;
	if(!r->data_remain)
		return addr;
	if(r->readbuf->size - r->rpos >= r->data_remain)
	{
		*len = r->data_remain;
		r->data_remain = 0;
		addr = &r->readbuf->buf[r->rpos];
		r->rpos += r->data_remain;
	}
	else
	{
		if(!r->binbuf)
		{
			r->binbufpos = 0;
			r->binbuf = buffer_create(r->len);
		}
		addr = r->binbuf->buf + r->binbufpos;
		size = r->data_remain;
		while(size)
		{
			uint32_t copy_size = r->readbuf->size - r->rpos;
			copy_size = copy_size >= size ? size:copy_size;
			memcpy(r->binbuf->buf + r->binbufpos,r->readbuf->buf + r->rpos,copy_size);
			size -= copy_size;
			r->rpos += copy_size;
			r->data_remain -= copy_size;
			r->binbufpos += copy_size;
			if(r->rpos >= r->readbuf->size && r->data_remain)
			{
				//当前buffer数据已经被读完,切换到下一个buffer
				r->rpos = 0;
				r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
			}
		}
	}
	return addr;
}

const char* rpk_read_string(rpacket_t r)
{
    if(PACKET_RAW(r))return NULL;
	uint32_t len = 0;
	return (const char *)rpk_read_binary(r,&len);
}

const void* rpk_read_binary(rpacket_t r,uint32_t *len)
{
	void *addr = 0;
	uint32_t size = 0;
    if(PACKET_RAW(r))
		return rpk_raw_read_binary(r,len);
	size = rpk_read_uint32(r);
	*len = size;
	if(!r->data_remain || r->data_remain < size)
		return addr;
	if(r->readbuf->size - r->rpos >= size)
	{
		addr = &r->readbuf->buf[r->rpos];
		r->rpos += size;
		r->data_remain -= size;
		if(r->rpos >= r->readbuf->size && r->data_remain)
		{
			//当前buffer数据已经被读完,切换到下一个buffer
			r->rpos = 0;
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
		}
	}
	else
	{
		//数据跨越了buffer边界,创建binbuf,将数据拷贝到binbuf中
		if(!r->binbuf)
		{
			r->binbufpos = 0;
			r->binbuf = buffer_create(r->len);
		}
		addr = r->binbuf->buf + r->binbufpos;
		while(size)
		{
			uint32_t copy_size = r->readbuf->size - r->rpos;
			copy_size = copy_size >= size ? size:copy_size;
			memcpy(r->binbuf->buf + r->binbufpos,r->readbuf->buf + r->rpos,copy_size);
			size -= copy_size;
			r->rpos += copy_size;
			r->data_remain -= copy_size;
			r->binbufpos += copy_size;
			if(r->rpos >= r->readbuf->size && r->data_remain)
			{
				//当前buffer数据已经被读完,切换到下一个buffer
				r->rpos = 0;
				r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
			}
		}

	}
	return addr;
}

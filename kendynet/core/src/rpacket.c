#include "rpacket.h"
#include "wpacket.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "common_define.h"
#include "atomic.h"

allocator_t rpacket_allocator = NULL;

rpacket_t rpk_create(buffer_t b,
                     uint32_t pos,
                     uint32_t pk_len,
                     uint8_t is_raw)
{
	rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
	struct packet *base = (struct packet*)r;
	r->binbuf = NULL;
	r->binbufpos = 0;
	base->buf = buffer_acquire(NULL,b);
	r->readbuf = buffer_acquire(NULL,b);
	r->len = pk_len;
	r->data_remain = r->len;
	base->begin_pos = pos;
	base->next.next = NULL;
	base->type = MSG_RPACKET;

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
	struct packet *base = (struct packet*)r;
	struct packet *other_base = (struct packet*)other;
	r->binbuf = NULL;
	r->binbufpos = 0;
	base->buf = buffer_acquire(NULL,other_base->buf);
	r->readbuf = buffer_acquire(NULL,other->readbuf);
	r->len = other->len;
	r->data_remain = other->len;
	base->begin_pos = other_base->begin_pos;
	base->next.next = NULL;
	base->type = MSG_RPACKET;
	base->raw = other_base->raw;
	r->rpos = other->rpos;
	return r;
}

static inline rpacket_t rpk_create_by_wpacket(struct wpacket *w)
{
	rpacket_t r = (rpacket_t)ALLOC(rpacket_allocator,sizeof(*r));
	struct packet *base = (struct packet*)r;
	struct packet *other_base = (struct packet*)w;
	r->binbuf = NULL;
	r->binbufpos = 0;
	base->buf = buffer_acquire(NULL,other_base->buf);
	r->readbuf = buffer_acquire(NULL,other_base->buf);
	base->raw = other_base->raw;
	base->begin_pos = other_base->begin_pos;
	base->next.next = NULL;
	base->type = MSG_RPACKET;
	if(base->raw)
	{
		r->len = w->data_size;
		r->rpos =other_base->begin_pos;
	}
	else
	{
		//这里的len只记录构造时wpacket的len,之后wpacket的写入不会影响到rpacket的len
		r->len = w->data_size - sizeof(r->len);
		r->rpos = (base->begin_pos + sizeof(r->len))%base->buf->capacity;
		if(r->rpos < base->begin_pos)
			r->readbuf = buffer_acquire(r->readbuf,r->readbuf->next);
	}
	r->data_remain = r->len;
	return r;
}

rpacket_t rpk_create_by_other(struct packet *p)
{
	if(p->type == MSG_RPACKET)
		return rpk_create_by_rpacket((rpacket_t)p);
	else if(p->type == MSG_WPACKET)
		return rpk_create_by_wpacket((wpacket_t)p);
	return NULL;
}



void      rpk_destroy(rpacket_t *r)
{
	//释放所有对buffer_t的引用
	buffer_release(&(*r)->base.buf);
	buffer_release(&(*r)->readbuf);
	buffer_release(&(*r)->binbuf);
	FREE(rpacket_allocator,*r);
	*r = 0;
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
			r->binbuf = buffer_create_and_acquire(NULL,r->len);
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
	uint32_t len = 0;
	if(r->base.raw)//raw类型的rpacket不支持读取字符串
		return rpk_raw_read_binary(r,&len);
	return (const char *)rpk_read_binary(r,&len);
}

const void* rpk_read_binary(rpacket_t r,uint32_t *len)
{
	void *addr = 0;
	uint32_t size = 0;
	if(r->base.raw)
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
			r->binbuf = buffer_create_and_acquire(NULL,r->len);
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

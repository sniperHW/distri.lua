#ifndef _RINGQUE_H
#define _RINGQUE_H

#include <stdint.h>
#include "kn_atomic.h"

static inline int32_t is_pow2(uint32_t size){
	return !(size&(size-1));
}

static inline uint32_t size_of_pow2(uint32_t size){
    if(is_pow2(size)) return size;
	size = size-1;
	size = size | (size>>1);
	size = size | (size>>2);
	size = size | (size>>4);
	size = size | (size>>8);
	size = size | (size>>16);
	return size + 1;
}

//单读单写循环队列
#define CARGO_CAPACITY 128

#define DECLARE_RINGQUE_1(NAME,TYPE)\
struct NAME##cargo{\
	TYPE cargo[CARGO_CAPACITY];\
	uint8_t size;\
};\
typedef struct NAME{\
	volatile uint32_t ridx;\
	uint32_t indexmask;\
	volatile uint32_t widx;\
	struct NAME##cargo data[];\
}*NAME;\
static inline void NAME##_push(NAME que,TYPE element)\
{\
	struct NAME##cargo *cargo = &que->data[que->widx];\
	cargo->cargo[cargo->size++] = element;\
	if(unlikely(cargo->size == CARGO_CAPACITY))\
	{\
		int32_t c=0;\
		uint32_t next_widx = (que->widx+1)&que->indexmask;\
		while(next_widx == que->ridx)\
		{\
			if(c < 4000){\
                ++c;\
                __asm__("pause");\
            }else{\
                struct timespec ts = { 0, 500000 };\
                nanosleep(&ts, NULL);\
            }\
		}\
		_FENCE;\
		que->widx = next_widx;\
	}\
}\
static inline int32_t NAME##_pop(NAME que,TYPE *ret)\
{\
	if(que->ridx == que->widx)\
		return -1;\
	_FENCE;\
	struct NAME##cargo *cargo = &que->data[que->ridx];\
	if(unlikely(cargo->size == 0)){\
		_FENCE;\
		que->ridx = (que->ridx+1)&que->indexmask;\
		if(que->ridx == que->widx)\
			return -1;\
		cargo = &que->data[que->ridx];\
	}\
	*ret = cargo->cargo[CARGO_CAPACITY-cargo->size];\
	--cargo->size;\
	return 0;\
}\
static inline NAME NAME##_new(uint32_t maxsize){\
	maxsize = size_of_pow2(maxsize);\
	NAME ringque = calloc(1,sizeof(*ringque)*sizeof(struct NAME##cargo)*maxsize);\
	ringque->indexmask = maxsize-1;\
	ringque->ridx = ringque->widx = 0;\
	return ringque;\
}

//多读多写循环队列
#define DECLARE_RINGQUE_N(NAME,TYPE)\
struct NAME##ringque_N_tls{\
	uint32_t rindex;\
	uint32_t windex;\
	TYPE data[CARGO_CAPACITY];\
};\
typedef struct NAME{\
	union{\
		uint32_t maximumrindex;\
		uint64_t pad1;\
	};\
	union{\
		uint32_t rindex;\
		uint64_t pad2;\
	};\
	union{\
		uint32_t windex;\
		uint64_t pad3;\
	};\
	uint32_t cap;\
	pthread_key_t         tls_key;\
	struct NAME##ringque_N_tls *data[0];\
}*NAME;\
static inline NAME new_##NAME(uint32_t cap){\
	cap = size_of_pow2(cap);\
	NAME q = calloc(1,sizeof(*q)+sizeof(struct NAME##ringque_N_tls)*cap);\
	q->maximumrindex = q->rindex = q->windex = 0;\
	q->cap = cap;\
	pthread_key_create(&q->tls_key,NULL);\
	return q;\
}\
static inline void NAME##_push(NAME q,TYPE val){\
     struct NAME##ringque_N_tls *local = (struct NAME##ringque_N_tls*)pthread_getspecific(q->tls_key);\
     if(!local){\
		local = calloc(1,sizeof(*local));\
		pthread_setspecific(q->tls_key,local);\
	}\
	local->data[local->windex++] = val;\
	if(local->windex >= CARGO_CAPACITY){\
		 uint32_t rindex,windex,next_windex,c;\
		 do{\
			c = 0;\
			do{\
				rindex = q->rindex;\
				windex = q->windex;\
				next_windex = (windex+1)%q->cap;\
				if(next_windex == rindex){\
					if(c < 4000){\
						++c;\
					__asm__("pause");\
					}else{\
						struct timespec ts = { 0, 500000 };\
						nanosleep(&ts, NULL);\
					}\
				}else\
					break;\
			}while(1);\
		}while(!COMPARE_AND_SWAP(&q->windex,windex,next_windex));\
		local->rindex = 0;\
		q->data[windex] = local;\
		c = 0;\
		while(!COMPARE_AND_SWAP(&q->maximumrindex,windex,next_windex)){\
			if(c < 4000){\
				++c;\
			__asm__("pause");\
			}else{\
				struct timespec ts = { 0, 500000 };\
				nanosleep(&ts, NULL);\
			}	\
		}\
		pthread_setspecific(q->tls_key,NULL);\
	}\
}\
static inline TYPE NAME##_pop(NAME q){\
	struct NAME##ringque_N_tls *local = (struct NAME##ringque_N_tls*)pthread_getspecific(q->tls_key);\
	if(local && local->rindex < local->windex){\
		return local->data[local->rindex++];\
	}\
	if(local) free(local);\
	uint32_t rindex,maximumrindex,c;\
	do{\
		c = 0;\
		do{\
			rindex = q->rindex;\
			maximumrindex = q->maximumrindex;\
			if(rindex == maximumrindex){\
				if(c < 4000){\
					++c;\
				__asm__("pause");\
				}else{\
					struct timespec ts = { 0, 500000 };\
					nanosleep(&ts, NULL);\
				}\
			}else{\
				break;\
			}\
		}while(1);\
		local = q->data[rindex];\
		if(COMPARE_AND_SWAP(&q->rindex,rindex,(rindex+1)%q->cap)){\
			pthread_setspecific(q->tls_key,local);\
			return local->data[local->rindex++];\
		}\
	}while(1);\
}
/*
//多读多写循环队列
typedef struct ringque_N{
	union{
		uint32_t maximumrindex;
		uint64_t pad1;
	};
	union{
		uint32_t rindex;
		uint64_t pad2;
	};
	union{
		uint32_t windex;
		uint64_t pad3;
	};
	uint32_t cap;	
	int32_t  data[0];	
}*ringque_N_t;


ringque_N_t new_ringque_N(uint32_t cap){
	cap = size_of_pow2(cap);
	ringque_N_t q = calloc(1,sizeof(*q)+sizeof(int)*cap);
	q->maximumrindex = q->rindex = q->windex = 0;
	q->cap = cap;
	return q;
}

void ringque_N_push(ringque_N_t q,int val){
     uint32_t rindex,windex,next_windex,c;
     do{
		c = 0;
		do{
			rindex = q->rindex;
			windex = q->windex;
			next_windex = (windex+1)%q->cap;
			if(next_windex == rindex){
				//not enough space for push
				if(c < 4000){
					++c;
                __asm__("pause");
				}else{
					struct timespec ts = { 0, 500000 };
					nanosleep(&ts, NULL);
				}				
			}else
				break;
		}while(1);
	}while(!COMPARE_AND_SWAP(&q->windex,windex,next_windex));
    q->data[windex] = val; 
    c = 0;
    while(!COMPARE_AND_SWAP(&q->maximumrindex,windex,next_windex)){
		if(c < 4000){
			++c;
		__asm__("pause");
		}else{
			struct timespec ts = { 0, 500000 };
			nanosleep(&ts, NULL);
		}	
	}
}

int ringque_N_pop(ringque_N_t q){
	uint32_t rindex,maximumrindex,c;
	int ret;
	do{
		c = 0;
		do{
			rindex = q->rindex;
			maximumrindex = q->maximumrindex;
			if(rindex == maximumrindex){
				//no element for pop
				if(c < 4000){
					++c;
				__asm__("pause");
				}else{
					struct timespec ts = { 0, 500000 };
					nanosleep(&ts, NULL);
				}
			}else{
				break;
			}
		}while(1);
		ret = q->data[rindex];
		if(COMPARE_AND_SWAP(&q->rindex,rindex,(rindex+1)%q->cap))
			return ret;
	}while(1);
}*/
#endif

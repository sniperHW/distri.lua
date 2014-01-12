#ifndef _LPROCESS_H
#define _LPROCESS_H

#include "minheap.h"
#include "uthread.h"
#include "dlist.h"
#include "llist.h"

/*
* 轻量级进程（用户级线程）及调度器,主要用于实现对用户友好的异步rpc调用
*/

//轻量级进程状态
enum
{
    _YIELD = 0,
    _SLEEP,
    _ACTIVE,
    _DIE,
    _START,
    _BLOCK,
};

typedef struct lprocess
{
    struct lnode   next;
    struct dnode   dblink;
    struct heapele _heapele;
    uthread_t      ut;
    void          *stack;
    uint8_t        status;
    uint32_t       timeout;
    void          *ud;
    void* (*star_func)(void *);

}*lprocess_t;

typedef struct lprocess_scheduler
{
    lprocess_t lp;
    int32_t    stack_size;
    minheap_t  _timer;
    int32_t    size;
}*lprocess_scheduler_t;

//创建一个lprocess，执行start_fun
int32_t lp_spawn(void*(*start_fun)(void*),void*arg);

//让当前lprocess休眠ms毫秒
void    lp_sleep(int32_t ms);

//当前lprocess让出执行权
void    lp_yield();

//阻塞当前lprocess(只能由其它lprocess或调度器唤醒)
void    lp_block();

void    lp_schedule();

#endif

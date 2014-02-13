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
*  一个线程安全的单向消息队列,提供多线程对队列的N读N写.
*  每个队列有一个由所有线程共享的公共队列,同时每个线程有自己私有的push/pop队列
*  一个线程往消息队列中push消息时,首先将消息push到自己私有的push队列中,然后判断当前
*  私有push队列的消息数量是否超过了预定的阀值,如果超过就将私有队列中的消息全部同步到
*  共享公共队列中.
*  当一个线程从消息队列中pop消息时,首先查看自己的私有pop队列中是否有消息,如果有直接从
*  私有消息队列中pop一个出来,如果私有消息队列中没有消息,则尝试从共享队列中将所有消息
*  同步到私有队列.
*
*  通过对私有和共享队列的划分,大量的减少了消息队列操作过程中锁的使用次数.
*
*  [注意1]
*  一个线程对同一个队列只能处于一种模式,
*  线程第一次对队列执行操作(写/读)，以后这个线程对这个队列就只能处于此模式下.
*  例如线程A第一次对队列执行了push操作，那么以后A对这个队列就只能执行push,
*  如果后面A对队列执行了pop将会引发错误
*
*  [注意2]
*  使用者只需调用new_msgque创建新的消息队列，队列内部维持了一个对使用线程的引用
*  计数，引用线程结束时会将计数减1,当所有引用线程结束后消息队列自行销毁.
*  
*  一个线程第一次对一个消息队列调用msgque_put,msgque_put_immeda,msgque_get时，将会
*  增加此线程对这个消息队列的引用计数
*/

#ifndef _MSGQUE_H
#define _MSGQUE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "refbase.h"
#include "dlist.h"
#include "llist.h"
#include "sync.h"

extern uint32_t msgque_flush_time;//默认心跳冲刷时间是50ms,用户可自己将该变量设置成合适的值

//用于销毁队列中残留的消息
typedef void (*item_destroyer)(void*);

typedef struct msg_que* msgque_t;

//默认销毁函数对残留元素执行free
void default_item_destroyer(void* item);


struct msg_que* new_msgque(uint32_t syn_size,item_destroyer);

/* push一条消息到local队列,如果local队列中的消息数量超过阀值执行同步，否则不同步
*  返回非0表示出错
*/
int8_t msgque_put(msgque_t,lnode *msg);

/* push一条消息到local队列,并且立即同步
*  返回非0表示出错
*/
int8_t msgque_put_immeda(msgque_t,lnode *msg);

/*从local队列pop一条消息,如果local队列中没有消息,尝试从共享队列同步消息过来
* 如果共享队列中没消息，最多等待timeout毫秒
*  返回非0表示出错
*/
int8_t msgque_get(msgque_t,lnode **msg,int32_t timeout);

int32_t msgque_len(msgque_t,int32_t timeout);


typedef void (*interrupt_function)(void*);
void   msgque_putinterrupt(msgque_t,void *ud,interrupt_function);
void   msgque_removeinterrupt(msgque_t);


//冲刷本线程持有的所有消息队列的local push队列
void   msgque_flush();

#ifdef MQ_HEART_BEAT
void   block_sigusr1();
void   unblock_sigusr1();
#endif

#endif

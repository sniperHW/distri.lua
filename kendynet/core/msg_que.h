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
*  对于无法以一定频率执行队列冲刷操作(将本地push队列中的消息同步到共享队列)的线程
*  为避免消费者出现饥饿务必使用push_now,该函数在将消息push到本地队列后立即执行同步操作
*/

#ifndef _MSG_QUE_H
#define _MSG_QUE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "refbase.h"
#include "double_link.h"
#include "link_list.h"
#include "sync.h"

//extern uint32_t msgque_flush_time;//默认心跳冲刷时间是10ms,用户可自己将该变量设置成合适的值

//用于销毁队列中残留的消息
typedef void (*item_destroyer)(void*);

typedef struct msg_que
{
        struct refbase      refbase;
        struct link_list    share_que;
        uint32_t            syn_size;
        volatile uint8_t    wait4destroy;
        pthread_key_t       t_key;
        mutex_t             mtx;
        struct double_link  blocks;
        item_destroyer      destroy_function;
}*msgque_t;

//默认销毁函数对残留元素执行free
void default_item_destroyer(void* item);

//增加对que的引用
static inline struct msg_que* msgque_acquire(msgque_t que){
    ref_increase((struct refbase*)que);
    return que;
}

//释放对que的引用,当计数变为0,que会被销毁
void msgque_release(msgque_t que);

struct msg_que* new_msgque(uint32_t syn_size,item_destroyer);

void delete_msgque(void*);

/*
* 请求关闭消息队列但不释放引用,要释放引用，主动调用msg_que_release,确保acquire和release的配对调用
* 关闭时，如果有线程等待在此消息队列中会被全部唤醒
*/
void close_msgque(msgque_t que);


/* push一条消息到local队列,如果local队列中的消息数量超过阀值执行同步，否则不同步
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_put(msgque_t,list_node *msg);

/* push一条消息到local队列,并且立即同步
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_put_immeda(msgque_t,list_node *msg);

/*从local队列pop一条消息,如果local队列中没有消息,尝试从共享队列同步消息过来
* 如果共享队列中没消息，最多等待timeout毫秒
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_get(msgque_t,list_node **msg,uint32_t timeout);


//冲刷本线程持有的所有消息队列的local push队列
void   msgque_flush();


#endif
 
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
*  对于无法以一定频率执行队列冲刷操作(将本地push队列中的消息同步到共享队列)的线程
*  为避免消费者出现饥饿务必使用push_now,该函数在将消息push到本地队列后立即执行同步操作
*/

#ifndef _MSG_QUE_H
#define _MSG_QUE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "refbase.h"
#include "double_link.h"
#include "link_list.h"
#include "sync.h"

//用于销毁队列中残留的消息
typedef void (*item_destroyer)(void*);

typedef struct msg_que
{
        struct refbase      refbase;
        struct link_list    share_que;
        uint32_t            syn_size;
        volatile uint8_t    wait4destroy;
        pthread_key_t       t_key;
        mutex_t             mtx;
        struct double_link  blocks;
        item_destroyer      destroy_function;
}*msgque_t;

//默认销毁函数对残留元素执行free
void default_item_destroyer(void* item);

//增加对que的引用
static inline struct msg_que* msgque_acquire(msgque_t que){
    ref_increase((struct refbase*)que);
    return que;
}

//释放对que的引用,当计数变为0,que会被销毁
void msgque_release(msgque_t que);

struct msg_que* new_msgque(uint32_t syn_size,item_destroyer);

void delete_msgque(void*);

/*
* 请求关闭消息队列但不释放引用,要释放引用，主动调用msg_que_release,确保acquire和release的配对调用
* 关闭时，如果有线程等待在此消息队列中会被全部唤醒
*/
void close_msgque(msgque_t que);


/* push一条消息到local队列,如果local队列中的消息数量超过阀值执行同步，否则不同步
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_put(msgque_t,list_node *msg);

/* push一条消息到local队列,并且立即同步
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_put_immeda(msgque_t,list_node *msg);

/*从local队列pop一条消息,如果local队列中没有消息,尝试从共享队列同步消息过来
* 如果共享队列中没消息，最多等待timeout毫秒
* 返回0正常，-1，队列已被请求关闭
*/
int8_t msgque_get(msgque_t,list_node **msg,uint32_t timeout);


//冲刷本线程持有的所有消息队列的local push队列
void   msgque_flush();


#endif

#include "msg_que.h"
#include <assert.h>
#include <stdio.h>
#include "SysTime.h"

enum{
    MSGQ_READ = 1,
    MSGQ_WRITE,
};

#ifdef MQ_HEART_BEAT
uint32_t msgque_flush_time = 50;
#endif
//每个线程都有一个per_thread_que与que关联
struct per_thread_que
{
    union{
        struct write_que{
            struct double_link_node pnode; //用于链入per_thread_struct->per_thread_que
            uint8_t     flag;//0,正常;1,阻止heart_beat操作,2,设置了冲刷标记
        }write_que;
        struct read_que{
            struct double_link_node bnode; //用于链入msg_que->blocks
            condition_t cond;
        }read_que;
    };
    struct link_list local_que;
	msgque_t que;
	uint8_t     mode;//MSGQ_READ or MSGQ_WRITE
};

//一个线程使用的所有msg_que所关联的per_thread_que
struct per_thread_struct
{
    struct double_link_node hnode;//用于链入heart_beat.thread_structs
	struct double_link  per_thread_que;
	pthread_t thread_id;
};

static pthread_key_t g_msg_que_key;
static pthread_once_t g_msg_que_key_once = PTHREAD_ONCE_INIT;

#ifdef MQ_HEART_BEAT
//心跳处理，用于定时向线程发送信号冲刷msgque
struct heart_beat
{
    struct double_link  thread_structs;
    mutex_t mtx;
};

static struct heart_beat *g_heart_beat;
static pthread_key_t g_heart_beat_key;
static pthread_once_t g_heart_beat_key_once;

static void* heart_beat_routine(void *arg){
	while(1){
	    mutex_lock(g_heart_beat->mtx);
	    struct double_link_node *dn = double_link_first(&g_heart_beat->thread_structs);
	    if(dn){
            while(dn != &g_heart_beat->thread_structs.tail)
            {
                struct per_thread_struct *pts =(struct per_thread_struct*)dn;
                pthread_kill(pts->thread_id,SIGUSR1);
                dn = dn->next;
            }
	    }
	    mutex_unlock(g_heart_beat->mtx);
		sleepms(msgque_flush_time);
	}
	return NULL;
}

void heart_beat_signal_handler(int sig);

static void heart_beat_once_routine(){
    pthread_key_create(&g_heart_beat_key,NULL);
    g_heart_beat = calloc(1,sizeof(*g_heart_beat));
    double_link_clear(&g_heart_beat->thread_structs);
    g_heart_beat->mtx = mutex_create();

    //注册信号处理函数
    struct sigaction sigusr1;
	sigusr1.sa_flags = 0;
	sigusr1.sa_handler = heart_beat_signal_handler;
	sigemptyset(&sigusr1.sa_mask);
	int status = sigaction(SIGUSR1,&sigusr1,NULL);
	if(status == -1)
	{
		printf("error sigaction\n");
		exit(0);
	}
	//创建一个线程以固定的频率触发冲刷心跳
	thread_run(heart_beat_routine,NULL);
}

static inline struct heart_beat* get_heart_beat()
{
	pthread_once(&g_heart_beat_key_once,heart_beat_once_routine);
    return g_heart_beat;
}
#endif

static void msg_que_once_routine(){
    pthread_key_create(&g_msg_que_key,NULL);
}

void delete_msgque(void *ud)
{
	msgque_t que = (msgque_t)ud;
    list_node *n;
    if(que->destroy_function){
        while((n = LINK_LIST_POP(list_node*,&que->share_que))!=NULL)
            que->destroy_function((void*)n);
    }
	mutex_destroy(&que->mtx);
	pthread_key_delete(que->t_key);
	free(que);
	printf("on_que_destroy\n");
}

static inline struct per_thread_struct* get_per_thread_struct()
{
	struct per_thread_struct *pts = (struct per_thread_struct*)pthread_getspecific(g_msg_que_key);
	if(!pts){
		pts = calloc(1,sizeof(*pts));
		double_link_clear(&pts->per_thread_que);
		pthread_setspecific(g_msg_que_key,(void*)pts);
		pts->thread_id = pthread_self();
#ifdef MQ_HEART_BEAT
		//关联到heart_beat中
        struct heart_beat *hb = get_heart_beat();
        mutex_lock(hb->mtx);
        double_link_push(&hb->thread_structs,&pts->hnode);
        mutex_unlock(hb->mtx);
#endif
	}
	return pts;
}

//获取本线程，与que相关的per_thread_que
static inline struct per_thread_que* get_per_thread_que(struct msg_que *que,uint8_t mode)
{
	struct per_thread_que *ptq = (struct per_thread_que*)pthread_getspecific(que->t_key);
	if(!ptq){
		ptq = calloc(1,sizeof(*ptq));
		ptq->que = que;
		ptq->mode = mode;
        if(mode == MSGQ_READ) ptq->read_que.cond = condition_create();
		pthread_setspecific(que->t_key,(void*)ptq);
	}
	return ptq;
}

void msgque_release(msgque_t que){
	struct per_thread_que *ptq = (struct per_thread_que*)pthread_getspecific(que->t_key);
	if(ptq){
        if(ptq->mode == MSGQ_WRITE){
            ptq->write_que.flag = 1;
            double_link_remove(&ptq->write_que.pnode);
            struct per_thread_struct *pts =  get_per_thread_struct();
            if(double_link_empty(&pts->per_thread_que)){
                pthread_setspecific(g_msg_que_key,NULL);
#ifdef MQ_HEART_BEAT
                //从heart_beat中移除
                struct heart_beat *hb = get_heart_beat();
                mutex_lock(hb->mtx);
                double_link_remove(&pts->hnode);
                mutex_unlock(hb->mtx);
#endif
                free(pts);
            }
        }
        //销毁local_que中所有消息
        list_node *n;
        if(que->destroy_function){
            while((n = LINK_LIST_POP(list_node*,&ptq->local_que))!=NULL)
                que->destroy_function((void*)n);
        }
        if(ptq->mode == MSGQ_READ) condition_destroy(&ptq->read_que.cond);
        free(ptq);
	}
    ref_decrease((struct refbase*)que);
}

void default_item_destroyer(void* item){
	free(item);
}

struct msg_que* new_msgque(uint32_t syn_size,item_destroyer destroyer)
{
    pthread_once(&g_msg_que_key_once,msg_que_once_routine);
    struct msg_que *que = calloc(1,sizeof(*que));
    pthread_key_create(&que->t_key,NULL);
    que->refbase.destroyer = delete_msgque;
    double_link_clear(&que->blocks);
    que->mtx = mutex_create();
    link_list_clear(&que->share_que);
    que->syn_size = syn_size;
    que->wait4destroy = 0;
	que->destroy_function = destroyer;
    return que;
}

void close_msgque(struct msg_que *que)
{
    mutex_lock(que->mtx);
    if(que->wait4destroy){
        mutex_unlock(que->mtx);
        return;
    }
    que->wait4destroy = 1;
    mutex_unlock(que->mtx);
    struct double_link_node *dbn = double_link_pop(&que->blocks);
    while(dbn){
        struct per_thread_que *ptq = (struct per_thread_que*)dbn;
        condition_signal(ptq->read_que.cond);
        dbn = double_link_pop(&que->blocks);
    }
}

//push消息并执行同步操作
static inline int8_t msgque_sync_push(struct per_thread_que *ptq)
{
    assert(ptq->mode == MSGQ_WRITE);
    if(ptq->mode != MSGQ_WRITE) return 0;
	struct msg_que *que = ptq->que;
	mutex_lock(que->mtx);
	if(que->wait4destroy){
        mutex_unlock(que->mtx);
        return -1;
	}
	uint8_t empty = link_list_is_empty(&que->share_que);
	link_list_swap(&que->share_que,&ptq->local_que);
	if(empty){
		struct double_link_node *l = double_link_pop(&que->blocks);
		if(l){
			//if there is a block per_thread_struct wake it up
            struct per_thread_que *block_ptq = (struct per_thread_que*)l;
			mutex_unlock(que->mtx);
            condition_signal(block_ptq->read_que.cond);
			return 0;
		}
	}
	mutex_unlock(que->mtx);
	return 0;
}

static inline int8_t msgque_sync_pop(struct per_thread_que *ptq,uint32_t timeout)
{
    assert(ptq->mode == MSGQ_READ);
    if(ptq->mode != MSGQ_READ) return 0;
    struct msg_que *que = ptq->que;
        mutex_lock(que->mtx);
        if(link_list_is_empty(&que->share_que) && timeout){
                uint32_t end = GetSystemMs() + timeout;
		while(link_list_is_empty(&que->share_que) && !que->wait4destroy){
                        double_link_push(&que->blocks,&ptq->read_que.bnode);
                        if(0 != condition_timedwait(ptq->read_que.cond,que->mtx,timeout)){
                                //timeout
                double_link_remove(&ptq->read_que.bnode);
                                break;
                        }
			uint32_t l_now = GetSystemMs();
			if(l_now < end) timeout = end - l_now;
			else break;//timeout
                }
        }
        if(!que->wait4destroy && !link_list_is_empty(&que->share_que))
        link_list_swap(&ptq->local_que,&que->share_que);
        mutex_unlock(que->mtx);
        if(que->wait4destroy) return -1;
        return 0;
}

static inline void before_put(struct per_thread_que *ptq)
{
	struct per_thread_struct *pts = get_per_thread_struct();
    double_link_push(&pts->per_thread_que,&ptq->write_que.pnode);
}

int8_t msgque_put(msgque_t que,list_node *msg)
{
	if(que->wait4destroy)return -1;
	struct per_thread_que *ptq = get_per_thread_que(que,MSGQ_WRITE);
	assert(ptq->mode == MSGQ_WRITE);
    if(ptq->mode != MSGQ_WRITE)return 0;
    ptq->write_que.flag = 1;
	before_put(ptq);
	LINK_LIST_PUSH_BACK(&ptq->local_que,msg);
    int8_t ret = 0;
    if(ptq->write_que.flag == 2 || link_list_size(&ptq->local_que) >= que->syn_size)
        ret = msgque_sync_push(ptq);
    ptq->write_que.flag = 0;
    return ret;
}

int8_t msgque_put_immeda(msgque_t que,list_node *msg)
{
	if(que->wait4destroy)return -1;
	struct per_thread_que *ptq = get_per_thread_que(que,MSGQ_WRITE);
	assert(ptq->mode == MSGQ_WRITE);
    if(ptq->mode != MSGQ_WRITE)return 0;
    ptq->write_que.flag = 1;
	before_put(ptq);
	if(msg) LINK_LIST_PUSH_BACK(&ptq->local_que,msg);
    int8_t ret = msgque_sync_push(ptq);
    ptq->write_que.flag = 0;
    return ret;
}

int8_t msgque_get(msgque_t que,list_node **msg,uint32_t timeout)
{
	if(que->wait4destroy)return -1;
	struct per_thread_que *ptq = get_per_thread_que(que,MSGQ_READ);
	assert(ptq->mode == MSGQ_READ);
    if(ptq->mode != MSGQ_READ)return 0;
	//本地无消息,执行同步
	if(timeout && link_list_is_empty(&ptq->local_que)){
		if(-1 == msgque_sync_pop(ptq,timeout))
			return -1;
	}
	*msg = LINK_LIST_POP(list_node*,&ptq->local_que);
	if(que->wait4destroy) return -1;
	return 0;
}

static inline void _flush_local(struct per_thread_que *ptq)
{
    assert(ptq->mode == MSGQ_WRITE);
    if(ptq->que->wait4destroy)return;
    if(ptq->mode != MSGQ_WRITE)return;
    if(link_list_is_empty(&ptq->local_que))return;
    msgque_sync_push(ptq);
}

void msgque_flush()
{
    struct per_thread_struct *pts = get_per_thread_struct();
    struct double_link_node *dln = double_link_first(&pts->per_thread_que);
    while(dln && dln != &pts->per_thread_que.tail){
        struct per_thread_que *ptq = (struct per_thread_que*)dln;
        ptq->write_que.flag = 1;_flush_local(ptq);ptq->write_que.flag = 0;
        dln = dln->next;
    }
}


#ifdef MQ_HEART_BEAT
void heart_beat_signal_handler(int sig)
{
    block_sigusr1();
	struct per_thread_struct *pts = get_per_thread_struct();
	struct double_link_node *dln = double_link_first(&pts->per_thread_que);
	while(dln && dln != &pts->per_thread_que.tail){
		struct per_thread_que *ptq = (struct per_thread_que*)dln;
        if(ptq->write_que.flag == 0){
            _flush_local(ptq);
		}else{
            ptq->write_que.flag = 2;
		}
		dln = dln->next;
	}
    unblock_sigusr1();
}

void   block_sigusr1()
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGUSR1);
    pthread_sigmask(SIG_BLOCK,&mask,NULL);
}

void   unblock_sigusr1()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK,&mask,NULL);
}
#endif

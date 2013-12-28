#include "msg_que.h"
#include <assert.h>
#include <stdio.h>
#include "SysTime.h"

enum{
	MSGQ_NONE = 0,
	MSGQ_READ = 1,
	MSGQ_WRITE = 2,
};

uint32_t msgque_flush_time = 50;

//每个线程都有一个per_thread_que与que关联
typedef struct per_thread_que
{
	union{
		struct write_que{
			struct double_link_node pnode; //用于链入per_thread_struct->per_thread_que
			uint8_t     flag;//0,正常;1,阻止heart_beat操作,2,设置了冲刷标记
		}write_que;
		struct read_que{
			struct double_link_node bnode; //用于链入msg_que->blocks或msg_que->can_interrupt
			condition_t cond;
			interrupt_function  notify_function;
			void*     ud;
		}read_que;
	};
	struct link_list local_que;
	msgque_t que;
	item_destroyer      destroy_function;
	uint8_t     mode;//MSGQ_READ or MSGQ_WRITE
}*ptq_t;

//一个线程使用的所有msg_que所关联的per_thread_que
typedef struct per_thread_struct
{
	struct double_link_node hnode;//用于链入heart_beat.thread_structs
	struct double_link  per_thread_que;
	pthread_t thread_id;
}*pts_t;

static pthread_key_t g_msg_que_key;
static pthread_once_t g_msg_que_key_once = PTHREAD_ONCE_INIT;

#ifdef MQ_HEART_BEAT
//心跳处理，用于定时向线程发送信号冲刷msgque
typedef struct heart_beat
{
	struct double_link  thread_structs;
	mutex_t mtx;
}*hb_t;

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
				pts_t pts =(pts_t)dn;
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
	double_link_init(&g_heart_beat->thread_structs);
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

static inline hb_t get_heart_beat()
{
	pthread_once(&g_heart_beat_key_once,heart_beat_once_routine);
	return g_heart_beat;
}
#endif


void delete_msgque(void  *arg)
{
	msgque_t que = (msgque_t)arg;
	list_node *n;
	if(que->destroy_function){
		while((n = LINK_LIST_POP(list_node*,&que->share_que))!=NULL)
			que->destroy_function((void*)n);
	}
	mutex_destroy(&que->mtx);
	pthread_key_delete(que->t_key);
	free(que);
#ifdef _DEBUG
	printf("on_que_destroy\n");
#endif
}

static inline pts_t get_per_thread_struct()
{
	pts_t pts = (pts_t)pthread_getspecific(g_msg_que_key);
	if(!pts){
		pts = calloc(1,sizeof(*pts));
		double_link_init(&pts->per_thread_que);
		pthread_setspecific(g_msg_que_key,(void*)pts);
		pts->thread_id = pthread_self();
#ifdef MQ_HEART_BEAT
		//关联到heart_beat中
		hb_t hb = get_heart_beat();
		mutex_lock(hb->mtx);
		double_link_push(&hb->thread_structs,&pts->hnode);
		mutex_unlock(hb->mtx);
#endif
	}
	return pts;
}

void delete_per_thread_struct(void *arg)
{
#ifdef MQ_HEART_BEAT
	block_sigusr1();
	pthread_setspecific(g_msg_que_key,NULL);
	pts_t pts = (pts_t)arg;
	//从heart_beat中移除
	hb_t hb = get_heart_beat();
	mutex_lock(hb->mtx);
	double_link_remove(&pts->hnode);
	mutex_unlock(hb->mtx);
	free(pts);
	unblock_sigusr1();
#else
	pthread_setspecific(g_msg_que_key,NULL);
	free(arg);
#endif
#ifdef _DEBUG
	printf("delete_per_thread_struct\n");
#endif
}

//获取本线程，与que相关的per_thread_que
static inline ptq_t get_per_thread_que(struct msg_que *que,uint8_t mode)
{
	ptq_t ptq = (ptq_t)pthread_getspecific(que->t_key);
	if(!ptq){
		ptq = calloc(1,sizeof(*ptq));
		ptq->destroy_function = que->destroy_function;
		ptq->mode = mode;
		ptq->que = que;
		ref_increase(&que->refbase);
		if(mode == MSGQ_READ) ptq->read_que.cond = condition_create();
		pthread_setspecific(que->t_key,(void*)ptq);
	}else if(ptq->mode == MSGQ_NONE)
		ptq->mode = mode;
	return ptq;
}

void delete_per_thread_que(void *arg)
{
#ifdef MQ_HEART_BEAT
	block_sigusr1();
#endif
	ptq_t ptq = (ptq_t)arg;
	if(ptq->mode == MSGQ_WRITE){
		double_link_remove(&ptq->write_que.pnode);
	}
	//销毁local_que中所有消息
	list_node *n;
	if(ptq->destroy_function){
		while((n = LINK_LIST_POP(list_node*,&ptq->local_que))!=NULL)
			ptq->destroy_function((void*)n);
	}
	if(ptq->mode == MSGQ_READ) condition_destroy(&ptq->read_que.cond);
	ref_decrease(&ptq->que->refbase);
	free(ptq);
#ifdef MQ_HEART_BEAT
	unblock_sigusr1();
#endif
	printf("delete_per_thread_que\n");
}

static void main_exit()
{
	pthread_exit(NULL);//强制主线程退出时执行正确的清理工作
}

static void msg_que_once_routine(){
	pthread_key_create(&g_msg_que_key,delete_per_thread_struct);
	atexit(main_exit);
}

void default_item_destroyer(void* item){
	free(item);
}

struct msg_que* new_msgque(uint32_t syn_size,item_destroyer destroyer)
{
	pthread_once(&g_msg_que_key_once,msg_que_once_routine);
	struct msg_que *que = calloc(1,sizeof(*que));
	pthread_key_create(&que->t_key,delete_per_thread_que);
	double_link_init(&que->blocks);
	que->mtx = mutex_create();
	que->refbase.destroyer = delete_msgque;
	link_list_clear(&que->share_que);
	que->syn_size = syn_size;
	que->destroy_function = destroyer;
	get_per_thread_que(que,MSGQ_NONE);
	return que;
}


void   msgque_putinterrupt(msgque_t que,void *ud,interrupt_function callback)
{
	mutex_lock(que->mtx);
	ptq_t ptq = get_per_thread_que(que,MSGQ_READ);
	ptq->read_que.ud = ud;
	ptq->read_que.notify_function = callback;
	double_link_push(&que->can_interrupt,&ptq->read_que.bnode);
	mutex_unlock(que->mtx);
}

void   msgque_removeinterrupt(msgque_t que)
{
	ptq_t ptq = get_per_thread_que(que,MSGQ_READ);
	if(ptq->read_que.bnode.next || ptq->read_que.bnode.pre){
		mutex_lock(que->mtx);
		double_link_remove(&ptq->read_que.bnode);
		mutex_unlock(que->mtx);
	}
	ptq->read_que.ud = NULL;
	ptq->read_que.notify_function = NULL;
}

//push消息并执行同步操作
static inline void msgque_sync_push(ptq_t ptq)
{
	assert(ptq->mode == MSGQ_WRITE);
	if(ptq->mode != MSGQ_WRITE) return;
	msgque_t que = ptq->que;
	mutex_lock(que->mtx);
	uint8_t empty = link_list_is_empty(&que->share_que);
	link_list_swap(&que->share_que,&ptq->local_que);
	if(empty){
		struct double_link_node *l = double_link_pop(&que->blocks);
		if(l){
			//if there is a block per_thread_struct wake it up
			ptq_t block_ptq = (ptq_t)l;
			mutex_unlock(que->mtx);
			condition_signal(block_ptq->read_que.cond);
		}
	}
	//对所有在can_interrupt中的元素调用回调
	while(!double_link_empty(&que->can_interrupt))
	{
		ptq_t ptq = (ptq_t)double_link_pop(&que->can_interrupt);
		ptq->read_que.notify_function(ptq->read_que.ud);
	}
	mutex_unlock(que->mtx);
}

static inline void msgque_sync_pop(ptq_t ptq,int32_t timeout)
{
	msgque_t que = ptq->que;
	mutex_lock(que->mtx);
	if(timeout < 0){
		if(link_list_is_empty(&que->share_que) && timeout){
			uint32_t end = GetSystemMs() + timeout;
			double_link_push(&que->blocks,&ptq->read_que.bnode);
			do{
				if(0 != condition_timedwait(ptq->read_que.cond,que->mtx,timeout)){
					//timeout
					double_link_remove(&ptq->read_que.bnode);
					break;
				}
				uint32_t l_now = GetSystemMs();
				if(l_now < end) timeout = end - l_now;
				else break;//timeout
			}while(link_list_is_empty(&que->share_que));
		}
	}
	else if(link_list_is_empty(&que->share_que))
	{
		double_link_push(&que->blocks,&ptq->read_que.bnode);
		do{	
			condition_wait(ptq->read_que.cond,que->mtx);
		}while(link_list_is_empty(&que->share_que));
	}
	if(!link_list_is_empty(&que->share_que))
		link_list_swap(&ptq->local_que,&que->share_que);
	mutex_unlock(que->mtx);
}

static inline int8_t _put(msgque_t que,list_node *msg,uint8_t type)
{
	ptq_t ptq = get_per_thread_que(que,MSGQ_WRITE);
	if(ptq->mode == MSGQ_READ && msg)
	{
		msgque_sync_pop(ptq,0);
		LINK_LIST_PUSH_BACK(&ptq->local_que,msg);
		return 0;
	}else if(ptq->mode == MSGQ_WRITE)
	{
		ptq->write_que.flag = 1;
		pts_t pts = get_per_thread_struct();
		double_link_push(&pts->per_thread_que,&ptq->write_que.pnode);
		if(msg)LINK_LIST_PUSH_BACK(&ptq->local_que,msg);

		if(type == 1)
			msgque_sync_push(ptq);
		else if(type == 2){
			if(ptq->write_que.flag == 2 || link_list_size(&ptq->local_que) >= que->syn_size)
				msgque_sync_push(ptq);
		}
		ptq->write_que.flag = 0;
		return 0;
	}
	return -1;
}

int8_t msgque_put(msgque_t que,list_node *msg)
{
	return _put(que,msg,2);
}

int8_t msgque_put_immeda(msgque_t que,list_node *msg)
{
	return _put(que,msg,1);
}

int8_t msgque_get(msgque_t que,list_node **msg,int32_t timeout)
{
	ptq_t ptq = get_per_thread_que(que,MSGQ_READ);
	assert(ptq->mode == MSGQ_READ);
	if(ptq->mode != MSGQ_READ)return -1;
	//本地无消息,执行同步
	if(link_list_is_empty(&ptq->local_que))
		msgque_sync_pop(ptq,timeout);
	*msg = LINK_LIST_POP(list_node*,&ptq->local_que);
	return 0;
}

static inline void _flush_local(ptq_t ptq)
{
	assert(ptq->mode == MSGQ_WRITE);
	if(ptq->mode != MSGQ_WRITE)return;
	if(link_list_is_empty(&ptq->local_que))return;
	msgque_sync_push(ptq);
}

void msgque_flush()
{
	pts_t pts = (pts_t)pthread_getspecific(g_msg_que_key);
	if(pts){
		struct double_link_node *dln = double_link_first(&pts->per_thread_que);
		while(dln && dln != &pts->per_thread_que.tail){
			ptq_t ptq = (ptq_t)dln;
			ptq->write_que.flag = 1;_flush_local(ptq);ptq->write_que.flag = 0;
			dln = dln->next;
		}
	}
}

#ifdef MQ_HEART_BEAT
void heart_beat_signal_handler(int sig)
{
	block_sigusr1();
	pts_t pts = (pts_t)pthread_getspecific(g_msg_que_key);
	if(pts){
		struct double_link_node *dln = double_link_first(&pts->per_thread_que);
		while(dln && dln != &pts->per_thread_que.tail){
			ptq_t ptq = (ptq_t)dln;
			if(ptq->write_que.flag == 0){
				_flush_local(ptq);
			}else{
				ptq->write_que.flag = 2;
			}
			dln = dln->next;
		}
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

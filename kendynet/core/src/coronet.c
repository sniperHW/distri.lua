#include "coronet.h"


struct msg_listen
{
	struct msg base;
	char       ip[32];
	int32_t    port;
};

int32_t coronet_listen(coronet_t c,const char *ip,int32_t port)
{
	if(c->flag == 1)return -1;
	struct msg_listen *msg = calloc(1,sizeof(*msg));
	msg->base.type = MSG_LISTEN;
	strcpy(msg->ip,ip);
	msg->port = port;
	if(0 != msgque_put_immeda(c->accptor_and_connector.mq_in,(list_node*)msg)){
		free(msg);
		return -1;
	}
	return 0;
}

struct msg_connect
{
	struct msg base;
	char       ip[32];
	int32_t    port;
	uint32_t   timeout;
};

int32_t coronet_connect(coronet_t c,const char *ip,int32_t port,uint32_t timeout)
{
	if(c->flag == 1)return -1;
	struct msg_connect *msg = calloc(1,sizeof(*msg));
	msg->base.type = MSG_CONNECT;
	strcpy(msg->ip,ip);
	msg->port = port;
	msg->timeout = timeout;
	if(0 != msgque_put_immeda(c->accptor_and_connector.mq_in,(list_node*)msg)){
		free(msg);
		return -1;
	}
	return 0;
}

struct msg_bind
{
	uint32_t send_timeout;
	uint32_t recv_timeout;
	int8_t   raw;
};

int32_t coronet_bind(coronet_t c,sock_ident sock,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout)
{
	if(c->flag == 1)return -1;
	struct msg_bind *msg = calloc(1,sizeof(*msg));
	msg->base.type = MSG_BIND;
    msg->base._ident = TO_IDENT(sock);
	msg->recv_timeout = recv_timeout;
	msg->send_timeout = send_timeout;
	msg->raw = raw;
	int32_t idx = rand()%c->poller_count;
	if(0 != msgque_put_immeda(c->netpollers[idx].mq_in,(list_node*)msg)){
		free(msg);
		return -1;
	}
	return 0;
}

static void do_on_accpet(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
	struct connection *c = new_conn(s,1);
	datasocket_t d = create_datasocket(c,INVALID_SOCK);
	msgque_t mq =  (msgque_t)ud;
	msg_t    msg = calloc(1,sizeof(msg));
	msg->type = MSG_CONNECT;
	msg->_ident = make_ident(&d->ref);
	if(0 != msgque_put_immeda(mq,(list_node*)msg)){
		release_datasocket(d);
		free(msg);
	}
}

static void do_on_connect(SOCK sock,struct sockaddr_in *addr_remote,void *ud,int err)
{
    if(sock == INVALID_SOCK){
        //connect failed
    }
    else
    {

    }
}

static int8_t do_process_packet(struct connection *c,rpacket_t r)
{
    datasocket_t d = (datasocket_t)c->usr_ptr;
    r->_ident = TO_IDENT(d->sident);
    if(0 != msgque_put(d->que,(list_node*)r))
        return 1;
    return 0;
}

static void process_msg(struct engine_struct *n,msg_t msg)
{
	if(msg->type == MSG_BIND)
	{


	}else if(msg->type == MSG_CONNECT)
	{

	}else if(msg->type == MSG_LISTEN)
	{

	}else if(msg->type == MSG_ACTIVE_CLOSE)
	{

	}
	if(msg->msg_destroy_function) msg->msg_destroy_function((void*)msg);
	else free(msg);
}


static void *mainloop(void *arg)
{
	printf("start io thread\n");
	struct engine_struct *n = (struct engine_struct *)arg;
	while(0 == n->flag)
	{
		for(;;){
			list_node *node = NULL;
			msgque_get(n->mq_in,&node,0);
			if(node)
			{
				msg_t msg = (msg_t)node;
				if(msg->type == MSG_WPACKET)
				{
					//发送数据包
				}else
				{
					process_msg(n,msg);
				}
			}
			else
				break;
		}
		n->netpoller->loop(n->netpoller,50);
	}
	return NULL;
}

static void mq_item_destroyer(void *ptr)
{
	msg_t _msg = (msg_t)ptr;
	if(_msg->type == MSG_RPACKET)
		rpk_destroy((rpacket_t*)&_msg);
	else if(_msg->type == MSG_WPACKET)
		wpk_destroy((wpacket_t*)&_msg);
	else
	{
		if(_msg->msg_destroy_function)
			_msg->msg_destroy_function(ptr);
		else
			free(ptr);
	}
}

coronet_t create_coronet(uint8_t  pollercount,
						 uint32_t coro_count,   //协程池的大小
						 coro_on_connect     _coro_on_connect,
						 coro_on_connected   _coro_on_connected,
						 coro_on_disconnect  _coro_on_disconnect,
						 coro_connect_failed _coro_connect_failed,
						 coro_process_packet _coro_process_packet)
{
	if(pollercount == 0 || coro_count)return NULL;
	if(!_coro_on_connect || !_coro_on_connected || !_coro_on_disconnect || !_coro_process_packet)
		return NULL;
	if(pollercount > MAX_NETPOLLER)
		pollercount = MAX_NETPOLLER;
	coronet_t c = calloc(1,sizeof(*c));
	if(!c) return NULL;
	c->poller_count = pollercount;
	c->coro_count = coro_count;
	c->mq_out = new_msgque(1024,mq_item_destroyer);
	c->_coro_on_connect = _coro_on_connect;
	c->_coro_on_connected = _coro_on_connected;
	c->_coro_on_disconnect = _coro_on_disconnect;
	c->_coro_process_packet = _coro_process_packet;
	c->_coro_connect_failed = _coro_connect_failed;
	//创建线程池
	c->accptor_and_connector.thread_engine = create_thread(THREAD_JOINABLE);
	c->accptor_and_connector.mq_in = new_msgque(1024,mq_item_destroyer);
	c->accptor_and_connector.netpoller = new_service();
	thread_start_run(c->accptor_and_connector.thread_engine,mainloop,(void*)&c->accptor_and_connector);
	int32_t i = 0;
	for(; i < pollercount;++i)
	{
		c->netpollers[i].thread_engine = create_thread(THREAD_JOINABLE);
		c->netpollers[i].mq_in = new_msgque(1024,mq_item_destroyer);
		c->netpollers[i].netpoller = new_service();
		thread_start_run(c->netpollers[i].thread_engine,mainloop,(void*)&c->netpollers[i]);
	}
	return c;
}

void stop_coronet(coronet_t c)
{
	if(c->flag == 0){
		c->flag = 1;
		c->accptor_and_connector.flag = 1;
		thread_join(c->accptor_and_connector.thread_engine);
		int32_t i = 0;
		for( ;i < c->poller_count; ++i){
			c->netpollers[i].flag = 1;
			thread_join(c->netpollers[i].thread_engine);
		}
	}
}

void destroy_coronet(coronet_t c)
{
	stop_coronet(c);
	destroy_thread(&c->accptor_and_connector.thread_engine);
	destroy_service(&c->accptor_and_connector.netpoller);
	int32_t i = 0;
	for( ;i < c->poller_count; ++i){
		destroy_thread(&c->netpollers[i].thread_engine);
		destroy_service(&c->netpollers[i].netpoller);
	}
	pthread_key_delete(c->t_key);
	free(c);
}

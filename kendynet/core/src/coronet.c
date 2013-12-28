#include "coronet.h"
#include "Socket.h"
#include "datasocket.h"
#include "SysTime.h"

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
	struct msg base;
	void   *ud;
	uint32_t send_timeout;
	uint32_t recv_timeout;
	int8_t raw;
};

int32_t coronet_bind(coronet_t c,sock_ident sock,void *ud,int8_t raw,uint32_t send_timeout,uint32_t recv_timeout)
{
	if(c->flag == 1)return -1;
	struct msg_bind *msg = calloc(1,sizeof(*msg));
	msg->base.type = MSG_BIND;
    msg->base._ident = TO_IDENT(sock);
	msg->recv_timeout = recv_timeout;
	msg->send_timeout = send_timeout;
	msg->raw = raw;
	msg->ud =  ud;
	int32_t idx = rand()%c->poller_count;
	if(0 != msgque_put_immeda(c->netpollers[idx].mq_in,(list_node*)msg)){
		free(msg);
		return -1;
	}
	return 0;
}

struct msg_connection
{
	struct msg  base;
	char        ip[32];
	int32_t     port;
	uint32_t    reason;
};

static void do_on_disconnect(struct connection *c,uint32_t reason)
{
	datasocket_t d = (datasocket_t)c->usr_ptr;
	if(d->que){
		struct msg_connection *msg = calloc(1,sizeof(*msg));
		msg->base._ident = TO_IDENT(d->sident);
		msg->base.type = MSG_DISCONNECTED;
		msg->reason = reason;
		get_addr_remote(d->sident,msg->ip,32);
		get_port_remote(d->sident,&msg->port);
		if(0 != msgque_put_immeda(d->que,(list_node*)msg))
			free(msg);
	}
	release_datasocket(d);
}

static void do_on_recv_timeout(struct connection *c)
{
	active_close(c);
}

static void do_on_send_timeout(struct connection *c)
{
	active_close(c);
}

static inline void new_connection(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
	struct connection *c = new_conn(sock,1);
	c->_on_disconnect = do_on_disconnect;
	datasocket_t d = create_datasocket(c,INVALID_SOCK);
	msgque_t mq =  (msgque_t)ud;
	struct msg_connection *msg = calloc(1,sizeof(*msg));
	msg->base._ident = TO_IDENT(d->sident);
	msg->base.type = MSG_CONNECT;
	get_addr_remote(d->sident,msg->ip,32);
	get_port_remote(d->sident,&msg->port);

	if(0 != msgque_put_immeda(mq,(list_node*)msg)){
		release_datasocket(d);
		free(msg);
	}
}

static void do_on_accpet(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
	new_connection(sock,addr_remote,ud);
}

static void do_on_connect(SOCK sock,struct sockaddr_in *addr_remote,void *ud,int err)
{
    if(sock == INVALID_SOCK){
		msgque_t mq =  (msgque_t)ud;
		//connect失败
		struct msg_connection *msg = calloc(1,sizeof(*msg));
		msg->base.type = MSG_CONNECT_FAIL;
		msg->reason = err;
		inet_ntop(INET,addr_remote,msg->ip,32);
		msg->port = ntohs(addr_remote->sin_port);		
		if(0 != msgque_put_immeda(mq,(list_node*)msg)){
			free(msg);
		}
    }
    else
		new_connection(sock,addr_remote,ud);
}

static int8_t do_process_packet(struct connection *c,rpacket_t r)
{
    datasocket_t d = (datasocket_t)c->usr_ptr;
    r->base._ident = TO_IDENT(d->sident);
    if(0 != msgque_put(d->que,(list_node*)r))
        return 1;
    return 0;
}


static void process_msg(struct engine_struct *n,msg_t msg)
{
	if(msg->type == MSG_BIND)
	{
		datasocket_t d = cast_2_datasocket(CAST_2_SOCK(msg->_ident));
		if(d){
			struct connection *c = d->c;
			socket_t s = get_socket_wrapper(c->socket);
			if(!s->engine){
				struct msg_bind *_msgbind = (struct msg_bind*)msg;
				c->raw = _msgbind->raw;
				d->usr_ptr = _msgbind->ud;
				if(0 != n->netpoller->bind(n->netpoller,c,do_process_packet,do_on_disconnect,
								   _msgbind->recv_timeout,do_on_recv_timeout,
								   _msgbind->send_timeout,do_on_send_timeout))
				{
					d->que = n->_coronet->mq_out;
					//绑定出错，直接关闭连接
					do_on_disconnect(c,0);
				}else{
					d->sndque = n->mq_in;
					//通知逻辑绑定成功
					struct msg_connection *tmsg = calloc(1,sizeof(*tmsg));
					tmsg->base._ident = TO_IDENT(d->sident);
					tmsg->base.type = MSG_ONCONNECTED;
					get_addr_remote(d->sident,tmsg->ip,32);
					get_port_remote(d->sident,&tmsg->port);
					if(0 != msgque_put_immeda(n->_coronet->mq_out,(list_node*)tmsg))
						free(tmsg);
				}
			}
			release_datasocket(d);
		}
	}else if(msg->type == MSG_CONNECT)
	{
		struct msg_connect *_msg = (struct msg_connect*)msg;
		if(0 != n->netpoller->connect(n->netpoller,_msg->ip,_msg->port,(void*)n->_coronet->mq_out,do_on_connect,_msg->timeout))
		{
			//connect失败
			struct msg_connection *tmsg = calloc(1,sizeof(*tmsg));
			tmsg->base.type = MSG_CONNECT_FAIL;
			tmsg->reason = errno;
			strcpy(tmsg->ip,_msg->ip);
			tmsg->port = _msg->port;
			if(0 != msgque_put_immeda(n->_coronet->mq_out,(list_node*)tmsg)){
				free(tmsg);
			}
		}
	}else if(msg->type == MSG_LISTEN)
	{
		struct msg_listen *_msg = (struct msg_listen*)msg;
		SOCK s = n->netpoller->listen(n->netpoller,_msg->ip,_msg->port,(void*)n->_coronet->mq_out,do_on_accpet);
		
		struct msg_connection *tmsg = calloc(1,sizeof(*tmsg));
		tmsg->base.type = MSG_LISTEN_RET;
		datasocket_t d = NULL;
		if(s != INVALID_SOCK)
		{
			d = create_datasocket(NULL,s);
			d->sndque = n->mq_in;
			tmsg->base._ident = TO_IDENT(d->sident);
		}else
			tmsg->reason = errno;
		strcpy(tmsg->ip,_msg->ip);
		tmsg->port = _msg->port;
		if(0 != msgque_put_immeda(n->_coronet->mq_out,(list_node*)tmsg)){
			if(d) release_datasocket(d);
			free(tmsg);
		}
	}else if(msg->type == MSG_ACTIVE_CLOSE)
	{
		datasocket_t d = cast_2_datasocket(CAST_2_SOCK(msg->_ident));
		if(d){
			active_close(d->c);
			release_datasocket(d);
		}
	}
	if(msg->msg_destroy_function) msg->msg_destroy_function((void*)msg);
	else free(msg);
}


static inline void process_send(struct engine_struct *e,wpacket_t wpk)
{
	datasocket_t d = cast_2_datasocket(CAST_2_SOCK(wpk->base._ident));
	if(d)
	{
		send_packet(d->c,wpk);
		release_datasocket(d);
	}else
	{
		//连接已失效丢弃wpk
		wpk_destroy(&wpk);
	}
}

static void notify_function(void *arg)
{
	netservice *n = (netservice*)arg;
	n->wakeup(n);
}

static void *mainloop(void *arg)
{
	printf("start io thread\n");
	struct engine_struct *n = (struct engine_struct *)arg;
	while(0 == n->flag)
	{
		uint32_t tick = GetSystemMs();
		uint32_t timeout = tick + 50;
		int8_t is_empty = 0;
		for(;tick < timeout;){
			list_node *node = NULL;
			msgque_get(n->mq_in,&node,0);
			if(node)
			{
				msg_t msg = (msg_t)node;
				if(msg->type == MSG_WPACKET)
				{
					//发送数据包
					process_send(n,(wpacket_t)msg);
				}else
				{
					process_msg(n,msg);
				}
			}
			else{
				is_empty = 1;
				break;
			}
			tick = GetSystemMs();
		}
		if(is_empty){
			//注册中断器，如果阻塞在loop里时mq_in收到消息会调用唤醒函数唤醒loop
			msgque_putinterrupt(n->mq_in,(void*)n->netpoller,notify_function);
			n->netpoller->loop(n->netpoller,100);
			msgque_removeinterrupt(n->mq_in);
		}
		else
			n->netpoller->loop(n->netpoller,0);
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
						 coro_on_connect     _coro_on_connect,
						 coro_on_connected   _coro_on_connected,
						 coro_on_disconnect  _coro_on_disconnect,
						 coro_connect_failed _coro_connect_failed,
						 coro_listen_ret     _coro_listen_ret,
						 coro_process_packet _coro_process_packet)
{
	if(pollercount == 0)return NULL;
	if(!_coro_on_connect || !_coro_on_connected || !_coro_on_disconnect || !_coro_process_packet)
		return NULL;
	if(pollercount > MAX_NETPOLLER)
		pollercount = MAX_NETPOLLER;
	coronet_t c = calloc(1,sizeof(*c));
	if(!c) return NULL;
	c->poller_count = pollercount;
	c->mq_out = new_msgque(1024,mq_item_destroyer);
	c->_coro_on_connect = _coro_on_connect;
	c->_coro_on_connected = _coro_on_connected;
	c->_coro_on_disconnect = _coro_on_disconnect;
	c->_coro_process_packet = _coro_process_packet;
	c->_coro_connect_failed = _coro_connect_failed;
	c->_coro_listen_ret = _coro_listen_ret;
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
	free(c);
}


int32_t close_datasock(sock_ident s)
{
	datasocket_t d = cast_2_datasocket(s);
	if(d)
	{
		int32_t ret = 0;
		if(!d->sndque){
			active_close(d->c);
			ret = 1;
		}
		else
		{
			msg_t msg = calloc(1,sizeof(*msg));
			msg->_ident = TO_IDENT(s);
			msg->type = MSG_ACTIVE_CLOSE;
			if(0 != msgque_put_immeda(d->sndque,(list_node*)msg))
			{
				free(msg);
				ret = -2;
			}
		}
		release_datasocket(d);
		return ret;
	}
	return -1;
}


static void dispatch_msg(coronet_t c,msg_t msg)
{
	if(msg->type == MSG_RPACKET)
	{
		rpacket_t rpk = (rpacket_t)msg;
		sock_ident sock = CAST_2_SOCK(rpk->base._ident);
		if(c->_coro_process_packet(c,sock,rpk))
			rpk_destroy(&rpk);
	}else{
		struct msg_connection *tmsg = (struct msg_connection*)msg;
		sock_ident sock = CAST_2_SOCK(tmsg->base._ident);
		if(msg->type == MSG_ONCONNECT){
			if(c->_coro_on_connect)
				c->_coro_on_connect(c,sock,tmsg->ip,tmsg->port);
			else
				close_datasock(sock);
		}
		else if(msg->type == MSG_ONCONNECTED){
			if(c->_coro_on_connected)
				c->_coro_on_connected(c,sock,tmsg->ip,tmsg->port);
			else
				close_datasock(sock);
		}
		else if(msg->type == MSG_DISCONNECTED && c->_coro_on_disconnect){
				c->_coro_on_disconnect(c,sock,tmsg->ip,tmsg->port,tmsg->reason);
		}
		else if(msg->type == MSG_CONNECT_FAIL && c->_coro_connect_failed)
			c->_coro_connect_failed(c,tmsg->ip,tmsg->port,tmsg->reason);
		else if(msg->type == MSG_LISTEN_RET && c->_coro_listen_ret)
			c->_coro_listen_ret(c,sock,tmsg->ip,tmsg->port,tmsg->reason);
		free(msg);
	}
}

void peek_msg(coronet_t c,uint32_t ms)
{
	uint32_t nowtick = GetSystemMs();
	uint32_t timeout = nowtick+ms;
	do{
		msg_t _msg = NULL;
		uint32_t sleeptime = timeout - nowtick;
		msgque_get(c->mq_out,(list_node**)&_msg,sleeptime);
		if(_msg) dispatch_msg(c,_msg);
		nowtick = GetSystemMs();
	}while(c->flag == 0 && nowtick < timeout);
}
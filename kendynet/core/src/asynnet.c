#include "asynnet.h"
#include "socket.h"
#include "asynsock.h"
#include "systime.h"

static void asyncb_disconnect(struct connection *c,uint32_t reason)
{
    asynsock_t d = (asynsock_t)c->usr_ptr;
	if(d->que){
		struct msg_connection *msg = calloc(1,sizeof(*msg));
		msg->base._ident = TO_IDENT(d->sident);
		msg->base.type = MSG_DISCONNECTED;
		msg->reason = reason;
		get_addr_remote(d->sident,msg->ip,32);
		get_port_remote(d->sident,&msg->port);
        if(0 != msgque_put_immeda(d->que,(lnode*)msg))
			free(msg);
	}
    asynsock_release(d);
}

static void asyncb_io_timeout(struct connection *c)
{
	active_close(c);
}

void new_connection(SOCK sock,struct sockaddr_in *addr_remote,void *ud)
{
	struct connection *c = new_conn(sock,1);
    c->cb_disconnect = asyncb_disconnect;
    asynsock_t d = asynsock_new(c,INVALID_SOCK);
	msgque_t mq =  (msgque_t)ud;
	struct msg_connection *msg = calloc(1,sizeof(*msg));
	msg->base._ident = TO_IDENT(d->sident);
	msg->base.type = MSG_CONNECT;
	get_addr_remote(d->sident,msg->ip,32);
	get_port_remote(d->sident,&msg->port);

    if(0 != msgque_put_immeda(mq,(lnode*)msg)){
        asynsock_release(d);
		free(msg);
	}
}


static void asyncb_connect(SOCK sock,struct sockaddr_in *addr_remote,void *ud,int err)
{
    if(sock == INVALID_SOCK){
		msgque_t mq =  (msgque_t)ud;
		//connect失败
		struct msg_connection *msg = calloc(1,sizeof(*msg));
		msg->base.type = MSG_CONNECT_FAIL;
		msg->reason = err;
		inet_ntop(INET,addr_remote,msg->ip,32);
		msg->port = ntohs(addr_remote->sin_port);		
        if(0 != msgque_put_immeda(mq,(lnode*)msg)){
			free(msg);
		}
    }
    else
		new_connection(sock,addr_remote,ud);
}

static int8_t asyncb_process_packet(struct connection *c,rpacket_t r)
{
    asynsock_t d = (asynsock_t)c->usr_ptr;
    MSG_IDENT(r) = TO_IDENT(d->sident);
    if(0 != msgque_put(d->que,(lnode*)r))
        return 1;
    return 0;
}


static void process_msg(struct poller_st *n,msg_t msg)
{
	if(msg->type == MSG_BIND)
	{
        asynsock_t d = cast_2_asynsock(CAST_2_SOCK(msg->_ident));
		if(d){
			struct connection *c = d->c;
			socket_t s = get_socket_wrapper(c->socket);
			if(!s->engine){
				struct msg_bind *_msgbind = (struct msg_bind*)msg;
				c->raw = _msgbind->raw;
				d->usr_ptr = _msgbind->ud;
                if(0 != n->netpoller->bind(n->netpoller,c,asyncb_process_packet,asyncb_disconnect,
                                   _msgbind->recv_timeout,asyncb_io_timeout,
                                   _msgbind->send_timeout,asyncb_io_timeout))
				{
					//绑定出错，直接关闭连接
                    asyncb_disconnect(c,0);
				}else{
					d->sndque = n->mq_in;
					//通知逻辑绑定成功
					struct msg_connection *tmsg = calloc(1,sizeof(*tmsg));
					tmsg->base._ident = TO_IDENT(d->sident);
					tmsg->base.type = MSG_ONCONNECTED;
					get_addr_remote(d->sident,tmsg->ip,32);
					get_port_remote(d->sident,&tmsg->port);
                    if(0 != msgque_put_immeda(d->que,(lnode*)tmsg))
						free(tmsg);
				}
			}
            asynsock_release(d);
		}
	}else if(msg->type == MSG_CONNECT)
	{
		struct msg_connect *_msg = (struct msg_connect*)msg;
        if(0 != n->netpoller->connect(n->netpoller,_msg->ip,_msg->port,MSG_USRPTR(_msg),asyncb_connect,_msg->timeout))
		{
			//connect失败
			struct msg_connection *tmsg = calloc(1,sizeof(*tmsg));
			tmsg->base.type = MSG_CONNECT_FAIL;
			tmsg->reason = errno;
			strcpy(tmsg->ip,_msg->ip);
			tmsg->port = _msg->port;
            msgque_t que = (msgque_t)MSG_USRPTR(_msg);
            if(0 != msgque_put_immeda(que,(lnode*)tmsg)){
				free(tmsg);
			}
		}
    }else if(msg->type == MSG_ACTIVE_CLOSE)
	{
        asynsock_t d = cast_2_asynsock(CAST_2_SOCK(msg->_ident));
		if(d){
			active_close(d->c);
            asynsock_release(d);
		}
	}
    if(MSG_FN_DESTROY(msg)) MSG_FN_DESTROY(msg)((void*)msg);
	else free(msg);
}


static inline void process_send(struct poller_st *e,wpacket_t wpk)
{
    asynsock_t d = cast_2_asynsock(CAST_2_SOCK(MSG_IDENT(wpk)));
	if(d)
	{
		send_packet(d->c,wpk);
        asynsock_release(d);
	}else		
        wpk_destroy(&wpk);//连接已失效丢弃wpk
}

static void notify_function(void *arg)
{
    ((netservice*)arg)->wakeup((netservice*)arg);
}

static void *mainloop(void *arg)
{
	printf("start io thread\n");
    struct poller_st *n = (struct poller_st *)arg;
	while(0 == n->flag)
	{
		uint32_t tick = GetSystemMs();
        uint32_t timeout = tick + 10;
		int8_t is_empty = 0;
		for(;tick < timeout;){
            lnode *node = NULL;
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
            n->netpoller->loop(n->netpoller,10);
			msgque_removeinterrupt(n->mq_in);
		}
		else
			n->netpoller->loop(n->netpoller,0);
	}
	return NULL;
}

void mq_item_destroyer(void *ptr)
{
    msg_t msg = (msg_t)ptr;
    if(msg->type == MSG_RPACKET)
        rpk_destroy((rpacket_t*)&msg);
    else if(msg->type == MSG_WPACKET)
        wpk_destroy((wpacket_t*)&msg);
	else
	{
        if(MSG_FN_DESTROY(msg))
            MSG_FN_DESTROY(msg)(ptr);
		else
			free(ptr);
	}
}


asynnet_t asynnet_new(uint8_t  pollercount)
{
	if(pollercount == 0)return NULL;
	if(pollercount > MAX_NETPOLLER)
		pollercount = MAX_NETPOLLER;
    asynnet_t asynet = calloc(1,sizeof(*asynet));
    if(!asynet) return NULL;
    asynet->poller_count = pollercount;
	//创建线程池
	int32_t i = 0;
	for(; i < pollercount;++i)
	{
        asynet->netpollers[i].poller_thd = create_thread(THREAD_JOINABLE);
        asynet->netpollers[i].mq_in = new_msgque(64,mq_item_destroyer);
        asynet->netpollers[i].netpoller = new_service();
        thread_start_run(asynet->netpollers[i].poller_thd,mainloop,(void*)&asynet->netpollers[i]);
	}
    return asynet;
}

void asynnet_stop(asynnet_t asynet)
{
    if(asynet->flag == 0){
        asynet->flag = 1;
		int32_t i = 0;
        for( ;i < asynet->poller_count; ++i){
            asynet->netpollers[i].flag = 1;
            thread_join(asynet->netpollers[i].poller_thd);
		}
	}
}

void asynnet_delete(asynnet_t asynet)
{
    asynnet_stop(asynet);
	int32_t i = 0;
    for( ;i < asynet->poller_count; ++i){
        destroy_thread(&asynet->netpollers[i].poller_thd);
        destroy_service(&asynet->netpollers[i].netpoller);
	}
    free(asynet);
}


int32_t asynsock_close(sock_ident s)
{
    asynsock_t d = cast_2_asynsock(s);
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
            if(0 != msgque_put_immeda(d->sndque,(lnode*)msg))
			{
				free(msg);
				ret = -2;
			}
		}
        asynsock_release(d);
		return ret;
	}
	return -1;
}

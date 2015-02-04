#include "kendynet_private.h"
#include "redisconn.h"
#include "kn_event.h"

enum{
	REDIS_CONNECTING = 1,
	REDIS_ESTABLISH,
	REDIS_CLOSE,
};

void kn_redisDisconnect(redisconn_t rc);

static void redisLibevRead(redisconn_t rc){
    redisAsyncHandleRead(rc->context);
}

static void redisLibevWrite(redisconn_t rc){
    redisAsyncHandleWrite(rc->context);
}

typedef void (*redis_cb)(redisconn_t,redisReply*,void *pridata);

struct privst{
	kn_dlist_node node;
	redisconn_t rc;
	void*       privdata;
	void (*cb)(redisconn_t,redisReply*,void *pridata);
};

static void destroy_redisconn(redisconn_t rc){
		//printf("destroy_redisconn\n");
		kn_dlist_node *node;
		while((node = kn_dlist_pop(&rc->pending_command))){
			struct privst *pri = ((struct privst*)node);
			pri->cb(rc,NULL,pri->privdata);
			free(node);	
		}
		free(rc);	
}

void kn_redisDisconnect(redisconn_t rc);

static void redis_on_active(handle_t s,int event){
	redisconn_t rc = (redisconn_t)s;
	do{
		if(rc->comm_head.status == REDIS_CONNECTING){
			int err = 0;
			socklen_t len = sizeof(err);
			if (getsockopt(rc->comm_head.fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
				rc->cb_connect(NULL,-1,rc->comm_head.ud);
				kn_redisDisconnect(rc);
				break;
			}
			if(err){
				errno = err;
				rc->cb_connect(NULL,errno,rc->comm_head.ud);
				kn_redisDisconnect(rc);
				break;
			}
			//connect success  
			rc->comm_head.status = REDIS_ESTABLISH;
			rc->cb_connect(rc,0,rc->comm_head.ud);			
		}else{
			if(event & EVENT_READ)
				redisLibevRead(rc);
			if(event & EVENT_WRITE)
				redisLibevWrite(rc);
/*#ifdef _LINUX			
			if((event & (EPOLLERR | EPOLLHUP)) || (event & (EPOLLRDHUP | EPOLLIN))){
				redisLibevRead(rc);
			}
			if(event & EPOLLOUT){
				redisLibevWrite(rc);
			}
#elif _BSD
			if(event & EVFILT_READ){
				redisLibevRead(rc);
			}
			if(event & EVFILT_WRITE){
				redisLibevWrite(rc);
			}			

#else

#error "un support platform!"				

#endif*/
		}
	}while(0);
	
	if(rc->comm_head.status == REDIS_CLOSE){
		destroy_redisconn(rc);
	}	
}

static void redisAddRead(void *privdata){
	redisconn_t con = (redisconn_t)privdata;
	kn_enable_read(con->e,(handle_t)con);
/*#ifdef _LINUX	
	int events = ((handle_t)con)->events | EPOLLIN;
	if(kn_event_mod(con->e,(handle_t)con,events) == 0)
		con->events = events;	
#elif _BSD
	int ret;
	if(con->events == 0)
		ret = kn_event_add(con->e,(handle_t)con,EVFILT_READ);
	else
		ret = kn_event_enable(con->e,(handle_t)con,EVFILT_READ);
	if(ret == 0) con->events |= EVFILT_READ;		
#else

#error "un support platform!"				

#endif*/

}

static void redisDelRead(void *privdata) {
	redisconn_t con = (redisconn_t)privdata;
	kn_disable_read(con->e,(handle_t)con);
}

static void redisAddWrite(void *privdata) {
	redisconn_t con = (redisconn_t)privdata;
	kn_enable_write(con->e,(handle_t)con);
}

static void redisDelWrite(void *privdata) {
	redisconn_t con = (redisconn_t)privdata;
	kn_disable_write(con->e,(handle_t)con);
}

static void redisCleanup(void *privdata) {
    redisconn_t con = (redisconn_t)privdata;
    if(con){
		if(con->comm_head.status == REDIS_ESTABLISH && con->cb_disconnected) 
			con->cb_disconnected(con,con->comm_head.ud);
		kn_event_del(con->e,(handle_t)con);	
		con->comm_head.status = REDIS_CLOSE;		
	} 
}

int kn_redisAsynConnect(engine_t p,
			const char *ip,unsigned short port,
			void (*cb_connect)(struct redisconn*,int err,void *ud),
			void (*cb_disconnected)(struct redisconn*,void *ud),
			void *ud)
{
    redisAsyncContext *c = redisAsyncConnect(ip, port);
    if(c->err) {
        printf("Error: %s\n", c->errstr);
        return -1;
    }
    redisconn_t con = calloc(1,sizeof(*con));
    con->context = c;
    con->comm_head.fd =  ((redisContext*)c)->fd;
    con->comm_head.on_events = redis_on_active;
    con->comm_head.type = KN_REDISCONN;
    con->comm_head.status = REDIS_CONNECTING; 
    con->cb_connect = cb_connect; 
    con->cb_disconnected = cb_disconnected;
    con->comm_head.ud = ud;
    con->e = p;
    kn_event_add(p,(handle_t)con,EVENT_READ | EVENT_WRITE);
    kn_dlist_init(&con->pending_command);	
    c->ev.addRead =  redisAddRead;
    c->ev.delRead =  redisDelRead;
    c->ev.addWrite = redisAddWrite;
    c->ev.delWrite = redisDelWrite;
    c->ev.cleanup =  redisCleanup;
    c->ev.data = con;		
    return 0; 											
}

static void kn_redisCallback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply *reply = r;
	redisconn_t rc = ((struct privst*)privdata)->rc;
	redis_cb cb = ((struct privst*)privdata)->cb;
	if(cb){
		cb(rc,reply,((struct privst*)privdata)->privdata);
	}
	kn_dlist_remove((kn_dlist_node*)privdata);
	free(privdata);
}

int kn_redisCommand(redisconn_t rc,const char *cmd,
					void (*cb)(redisconn_t,redisReply*,void *pridata),void *pridata)
{
	struct privst *privst = NULL;
	if(cb){
		privst = calloc(1,sizeof(*privst));
		privst->rc = rc;
		privst->cb = cb;
		privst->privdata = pridata;
	}
	int status = redisAsyncCommand(rc->context, privst?kn_redisCallback:NULL,privst,cmd);
	if(status != REDIS_OK){
		if(privst) free(privst);
	}else{
		if(privst) kn_dlist_push(&rc->pending_command,(kn_dlist_node*)privst);
	}	
	return status;
}
					

void kn_redisDisconnect(redisconn_t rc){
	redisAsyncDisconnect(rc->context);
}


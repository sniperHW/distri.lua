#include "kn_proactor.h"
#include "redisconn.h"


void kn_redisDisconnect(redisconn_t rc);

static void redisLibevRead(redisconn_t rc){
    redisAsyncHandleRead(rc->context);
}

static void redisLibevWrite(redisconn_t rc){
    redisAsyncHandleWrite(rc->context);
}

void kn_redisDisconnect(redisconn_t rc);

static void redis_on_active(kn_fd_t s,int event){
	redisconn_t rc = (redisconn_t)s;
	if(rc->state == REDIS_CONNECTING){
		int err = 0;
		socklen_t len = sizeof(err);
		if (getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
			rc->cb_connect(rc,-1);
			//kn_closefd(s);
			kn_redisDisconnect(rc);
			return;
		}
		if(err){
			errno = err;
			rc->cb_connect(rc,errno);
			kn_redisDisconnect(rc);
			//kn_closefd(s);
			return;
		}
		//connect success  
		rc->state = REDIS_ESTABLISH;
		rc->cb_connect(rc,0);			
	}else{
		if(event & (EPOLLERR | EPOLLHUP)){
			kn_redisDisconnect(rc);	
			return;
		}
		kn_ref_acquire(&rc->base.ref);
		if(event & (EPOLLRDHUP | EPOLLIN)){
			//printf("read active\n");
			redisLibevRead(rc);
		}
		if((event & EPOLLOUT) && rc->state != REDIS_CLOSE){
			//printf("write active\n");
			redisLibevWrite(rc);
		}
		kn_ref_release(&rc->base.ref);	
	}
}

typedef void (*redis_cb)(redisconn_t,redisReply*,void *pridata);

struct privst{
	kn_dlist_node node;
	redisconn_t rc;
	void*       privdata;
	void (*cb)(redisconn_t,redisReply*,void *pridata);
};

static void redisconn_destroy(void *ptr){
	redisconn_t conn = (redisconn_t)((char*)ptr - sizeof(kn_dlist_node));
	kn_dlist_node *node;
	while((node = kn_dlist_pop(&conn->pending_command))){
		struct privst *pri = ((struct privst*)node);
		pri->cb(conn,NULL,pri->privdata);
		free(node);	
	}
	free(ptr);
}

static void redisAddRead(void *privdata){
	redisconn_t con = (redisconn_t)privdata;
	kn_proactor_t p = con->base.proactor;
	p->addRead(p,(kn_fd_t)con);
}

static void redisDelRead(void *privdata) {
	redisconn_t con = (redisconn_t)privdata;
	kn_proactor_t p = con->base.proactor;
	p->delRead(p,(kn_fd_t)con);
}

static void redisAddWrite(void *privdata) {
	//printf("redisAddWrite\n");
	redisconn_t con = (redisconn_t)privdata;
	kn_proactor_t p = con->base.proactor;
	p->addWrite(p,(kn_fd_t)con);
}

static void redisDelWrite(void *privdata) {
	//printf("redisDelWrite\n");
	redisconn_t con = (redisconn_t)privdata;
	kn_proactor_t p = con->base.proactor;
	p->delWrite(p,(kn_fd_t)con);
}

static void redisCleanup(void *privdata) {
    redisconn_t con = (redisconn_t)privdata;
    if(con){
		if(con->state == REDIS_ESTABLISH && con->cb_disconnected)
			con->cb_disconnected(con);		
		kn_proactor_t p = con->base.proactor;
		p->UnRegister(p,(kn_fd_t)con);
		con->state = REDIS_CLOSE;
		kn_closefd((kn_fd_t)con);
	} 
}


int kn_redisAsynConnect(struct kn_proactor *p,
						const char *ip,unsigned short port,
						void (*cb_connect)(struct redisconn*,int err),
						void (*cb_disconnected)(struct redisconn*)
						)
{
	redisAsyncContext *c = redisAsyncConnect(ip, port);
    if(c->err) {
        printf("Error: %s\n", c->errstr);
        return -1;
    }
    redisconn_t con = calloc(1,sizeof(*con));
    con->context = c;
	con->base.fd =  ((redisContext*)c)->fd;
	con->base.on_active = redis_on_active;
	con->base.type = REDISCONN;
	con->state = REDIS_CONNECTING; 
	con->cb_connect = cb_connect; 
	con->cb_disconnected = cb_disconnected;
	kn_dlist_init(&con->pending_command);
    kn_ref_init(&con->base.ref,redisconn_destroy);
	
    c->ev.addRead =  redisAddRead;
    c->ev.delRead =  redisDelRead;
    c->ev.addWrite = redisAddWrite;
    c->ev.delWrite = redisDelWrite;
    c->ev.cleanup =  redisCleanup;
    c->ev.data = con;		
	if(p->Register(p,(kn_fd_t)con) == 0){
		return 0;
	}else{
		redisAsyncFree(c);
		free(con);
		//kn_closefd((kn_fd_t)c);
		return -1;
	}    											
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


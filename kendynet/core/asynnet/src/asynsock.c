#include "asynsock.h"
#include "common_define.h"
#include "log.h"
#include "sync.h"

/*static mutex_t mtx;
static struct dlist   asynsock_pool;

static pthread_key_t g_asynsock_key;
static pthread_once_t g_asynsock_key_once = PTHREAD_ONCE_INIT;



static void expand_asynsock_pool()
{
	int i = 0;
	for(; i < 4096; ++i)
	{
		asynsock_t sock = calloc(1,sizeof(*sock));
		dlist_push(&asynsock_pool,(struct dnode*)sock);
	}
}

static void asynsock_once_routine(){
	pthread_key_create(&g_asynsock_key,NULL);
    dlist_init(&asynsock_pool);
	mtx = mutex_create();
	expand_asynsock_pool();
}*/


static inline asynsock_t get_asynsock(){
	/*pthread_once(&g_asynsock_key_once,asynsock_once_routine);
	asynsock_t ret = NULL;
	mutex_lock(mtx);
	if(dlist_empty(&asynsock_pool))
		expand_asynsock_pool();
	char *tmp = (char*)dlist_pop(&asynsock_pool);
	ret = (asynsock_t)(tmp - sizeof(struct refbase)); 
	mutex_unlock(mtx);
	memset(ret,0,sizeof(*ret));
	return ret;*/
	return (asynsock_t)calloc(1,sizeof(struct asynsock));
}

static inline void put_asynsock(asynsock_t sock){
	/*mutex_lock(mtx);
	dlist_push(&asynsock_pool,(struct dnode*)sock);
	mutex_unlock(mtx);*/
	free(sock);
}


void asynsock_destroy(void *ptr)
{
    asynsock_t sock = (asynsock_t)ptr;
	if(sock->c) 
		release_conn(sock->c);
	else if(sock->s != INVALID_SOCK) 
		CloseSocket(sock->s);
	put_asynsock(sock);
}

asynsock_t asynsock_new(struct connection *c,SOCK s)
{
    asynsock_t sock = get_asynsock();
    ref_init(&sock->ref,type_asynsock,asynsock_destroy,1);
    if(c){
		sock->c = c;
        c->usr_ptr = sock;
    }
	else if(s != INVALID_SOCK)
		sock->s = s;
	else
	{
		put_asynsock(sock);
		return NULL;
	}
    (*(ident*)&sock->sident) = make_ident(&sock->ref);
	return sock;
}

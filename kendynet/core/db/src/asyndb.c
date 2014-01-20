#include "asyndb.h"
#include "redis/asynredis.h"

int32_t asyndb_sendresult(msgsender sender,db_result_t result)
{
	//这里暂时只考虑sender是一个msgdisp的情况
	if(is_type(TO_IDENT(sender),type_msgdisp))
	{
		msgdisp_t disp = get_msgdisp(sender);
		wpacket_t wpk = wpk_create(64,0);
		wpk_write_uint16(wpk,ASYNDB_RESILT);
		wpk_write_uint32(wpk,(uint32_t)result);
		push_msg(NULL,disp,rpk_create_by_other((struct packet*)wpk));
		wpk_destroy(&wpk);
		return 0;	
	}
	return -1;
}

void request_destroyer(void *ptr)
{
	free_dbrequest((db_request_t)ptr);
}

asyndb_t new_asyndb()
{
	struct asynredis *redis = calloc(1,sizeof(*redis));
	llist_init(&redis->workers);
	redis->mq =  new_msgque(32,request_destroyer);
	redis->base.connectdb = redis_connectdb;
	redis->base.request = redis_request;
	return (asyndb_t)redis;
}

void free_asyndb(asyndb_t asyndb)
{
	
	struct asynredis *redis = (struct asynredis*)asyndb;
	struct lnode *n = llist_head(&redis->workers);
	while(n)
	{
		struct redis_worker *worker = (struct redis_worker*)n;
		worker->stop = 1;
		thread_join(worker->worker);
	}
	while((n = llist_pop(&redis->workers)) != NULL)
	{
		struct redis_worker *worker = (struct redis_worker*)n;
		redisFree(worker->context);
		destroy_thread(&worker->worker);
		free(worker);
	}
	free(asyndb);
} 

db_result_t new_dbresult(const char *result_str ,DB_CALLBACK callback ,int32_t err,void *ud)
{
	db_result_t result = calloc(1,sizeof(*result));
	result->callback = callback;
	result->err = err;
	result->ud = ud;
	if(result_str){
		result->result_str = calloc(1,strlen(result_str)+1);
		strcpy(result->result_str,result_str);
	}
	return result;
}


void     free_dbresult(db_result_t result)
{
	if(result->result_str) free(result->result_str);
	free(result);
}

db_request_t  new_dbrequest(uint8_t type,const char *req_string,DB_CALLBACK callback,void *ud,msgsender sender)
{
	if(!req_string) return NULL;
	if(type == db_get && !callback) return NULL;
	db_request_t request = calloc(1,sizeof(*request));
	request->type = type;
	request->callback = callback;
	request->sender = sender;
	request->query_str = calloc(1,strlen(req_string)+1);
	request->ud = ud;
	strcpy(request->query_str,req_string);
	return request;
}

void free_dbrequest(db_request_t req)
{
	free(req->query_str);
	free(req);
}

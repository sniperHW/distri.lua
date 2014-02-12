#include "../asyndb.h"
#include "asynredis.h"
#include "asynnet/asynnet.h"


/*db_result_t rpk_read_dbresult(rpacket_t r)
{
	return (db_result_t)rpk_read_pointer(r);
}

static inline wpacket_t create_dbresult_packet(db_result_t result)
{

	wpacket_t wpk = wpk_create(64,0);
	wpk_write_uint16(wpk,CMD_DB_RESULT);
	wpk_write_pointer(wpk,(void*)result);
	return wpk;
}*/

void asyndb_sendresult(msgdisp_t recver,db_result_t result)
{
	//msgdisp_t disp = get_msgdisp(sender);
	//wpacket_t wpk = create_dbresult_packet(result);
	if(0 != send_msg(NULL,recver,(msg_t)result))
		free_dbresult(result);
}

void request_destroyer(void *ptr)
{
	free_dbrequest((db_request_t)ptr);
}

asyndb_t new_asyndb()
{
	struct asynredis *redis = calloc(1,sizeof(*redis));
	llist_init(&redis->workers);
	redis->mtx = mutex_create();
	redis->mq =  new_msgque(32,request_destroyer);
	redis->base.connectdb = redis_connectdb;
	redis->base.request = redis_request;
	return (asyndb_t)redis;
}

void free_asyndb(asyndb_t asyndb)
{
	
	struct asynredis *redis = (struct asynredis*)asyndb;
	mutex_lock(redis->mtx);
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
	mutex_unlock(redis->mtx);
	mutex_destroy(&redis->mtx);
	free(asyndb);
} 

db_result_t new_dbresult(void *result_set,DB_CALLBACK callback ,int32_t err,void *ud)
{
	db_result_t result = calloc(1,sizeof(*result));
	MSG_TYPE(result) = MSG_DB_RESULT;
	result->callback = callback;
	result->err = err;
	result->ud = ud;
	result->result_set = result_set;
	return result;
}


void free_dbresult(db_result_t result)
{
	redisReply *r = (redisReply*)result->result_set;
	if(r) freeReplyObject(r);
	free(result);
}

db_request_t  new_dbrequest(uint8_t type,const char *req_string,DB_CALLBACK callback,void *ud,msgdisp_t sender)
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

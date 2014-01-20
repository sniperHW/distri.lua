#include "asyndb.h"
#include "asynredis.h"
#include "cmd.h"
#include "asynnet.h"


db_result_t rpk_read_dbresult(rpacket_t r)
{
	uint8_t local = rpk_read_uint8(r);
	if(local){
		return (db_result_t)rpk_read_pointer(r);
	}else{
		DB_CALLBACK callback = (DB_CALLBACK)rpk_read_pointer(r);
		void *ud = rpk_read_pointer(r);
		const char *str = rpk_read_string(r);
		int32_t err = (int32_t)rpk_read_uint32(r);
		return new_dbresult(str,callback,err,ud);
	}
}

static inline wpacket_t create_dbresult_packet(int8_t local,db_result_t result)
{

	wpacket_t wpk = wpk_create(64,0);
	wpk_write_uint16(wpk,CMD_DB_RESULT);
	if(local){
		wpk_write_uint8(wpk,1);
		wpk_write_pointer(wpk,(void*)result);
	}else{
		wpk_write_uint8(wpk,0);
		wpk_write_pointer(wpk,(void*)result->callback);
		wpk_write_pointer(wpk,(void*)result->ud);
		wpk_write_string(wpk,result->result_str);
		wpk_write_uint32(wpk,result->err);
	}
	return wpk;
}

int32_t asyndb_sendresult(msgsender sender,db_result_t result)
{
	//这里暂时只考虑sender是一个msgdisp的情况
	if(is_type(TO_IDENT(sender),type_msgdisp))
	{
		msgdisp_t disp = get_msgdisp(sender);
		wpacket_t wpk = create_dbresult_packet(1,result);
		push_msg(NULL,disp,rpk_create_by_other((struct packet*)wpk));
		wpk_destroy(&wpk);
		return 0;	
	}else
	{
		wpacket_t wpk = create_dbresult_packet(1,result);
		asyn_send(CAST_2_SOCK(sender),wpk);
		free_dbresult(result);
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

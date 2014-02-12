#include "asynredis.h"

void dorequest(struct redis_worker *worker,db_request_t req)
{
	while(1){
		redisContext *c = worker->context; 
		redisReply *r = redisCommand(c,req->query_str);
		if(r){
			if(req->type == db_get){
				db_result_t result = NULL;	
				if(r->type == REDIS_REPLY_NIL)
				{
					result = new_dbresult(r,req->callback,-1,req->ud);
				}else
				{
					result = new_dbresult(r,req->callback,0,req->ud);
				}	
				asyndb_sendresult(req->sender,result);
			}else if(req->type == db_set){
				if(req->callback){
					//需要result
					db_result_t result = NULL;
					if(!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK") == 0))
						result = new_dbresult(r,req->callback,-1,req->ud);
					else
						result = new_dbresult(r,req->callback,0,req->ud);
					asyndb_sendresult(req->sender,result);
				}		
			}
			break;
		}else
		{
			redisFree(c);
			worker->context = redisConnect(worker->ip,worker->port);
		}

	 //if(r) freeReplyObject(r);	
	}
	free_dbrequest(req);
}


static void *worker_main(void *ud){

	struct redis_worker *worker = (struct redis_worker*)ud;
	while(worker->stop == 0){
		db_request_t req = NULL;
        msgque_get(worker->mq,(lnode**)&req,10);
        if(req)
        	dorequest(worker,req);
	}
    return NULL;
}


int32_t redis_connectdb(asyndb_t asyndb,const char *ip,int32_t port)
{
	if(!asyndb) return -1;
	struct asynredis *redis = (struct asynredis*)asyndb;
	redisContext *c = redisConnect(ip,port);
	if(c->err){
		redisFree(c);
		return -1;
	}

	struct redis_worker *worker = calloc(1,sizeof(*worker));
	worker->context = c;
	worker->mq = redis->mq;
	worker->worker = create_thread(THREAD_JOINABLE);
	worker->stop = 0;
	strcpy(worker->ip,ip);
	worker->port = port;
	mutex_lock(redis->mtx);
	LLIST_PUSH_BACK(&redis->workers,worker);
	mutex_unlock(redis->mtx);
	thread_start_run(worker->worker,worker_main,(void*)worker);
	return 0;
}


int32_t redis_request(asyndb_t asyndb,db_request_t req)
{
	if(!asyndb) return -1;
	if(req->type == db_get && !req->callback) return -1;
	struct asynredis *redis = (struct asynredis*)asyndb;
	if(0 != msgque_put_immeda(redis->mq,(lnode*)req))
	{	
		free_dbrequest(req);
		printf("push request error\n");	
		return -1;
	}
	return 0;
}

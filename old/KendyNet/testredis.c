#include "kendynet.h"
#include "kn_time.h"
#include <stdio.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"

static int count = 0;
static volatile int stop = 0;

static void redisget_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(!reply) return;
	if(reply->type != REDIS_REPLY_NIL){
		++count;
		//freeReplyObject(reply);
	}else{
		printf("error:%d,%d\n",reply->type,(int)pridata);
	}
	char buf[512];
	int key = rand()%10000;
	snprintf(buf,512,"get key%d",key+1);
	kn_redisCommand(rc,buf,redisget_cb,(void*)key);		
	//kn_redisDisconnect(rc);
}

static int c = 0;
static void redisset_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(!reply) return;
	if(++c >= 1000000) stop = 1;
	//iif(reply->type != REDIS_REPLY_NIL){
	//printf("%d\n",reply->type);
}


static void cb_redis_disconnected(redisconn_t rc,void *_){
	(void)_;
	printf("disconnected\n");
}


static void cb_redis_connect(redisconn_t rc,int err,void *_){
	(void)_;
	if(err == 0){
		printf("connect success\n");
		
		int i = 0;
		/*for( ; i < 1000000; ++i){
			char buf[512];
			snprintf(buf,512,"set key%d %d",i+1,i+1);
			kn_redisCommand(rc,buf,redisset_cb,NULL);
		}*/
		//发出1000个get
		i = 0;
		for(; i < 1000; ++i){
			char buf[512];
			int key = rand()%10000;
			snprintf(buf,512,"get key%d",key+1);
			kn_redisCommand(rc,buf,redisget_cb,(void*)key);
		}
	}else{
		printf("connect fail:%d\n",err);
	}	
}



int main(){
	kn_proactor_t p = kn_new_proactor();
	if(0 != kn_redisAsynConnect(p,"127.0.0.1",6379,cb_redis_connect,cb_redis_disconnected,NULL)){
		printf("kn_redisAsynConnect failed\n");
		return 0;
	}
	uint32_t tick = kn_systemms();
	for(;stop==0;){
		kn_proactor_run(p,50);
		uint32_t now = kn_systemms();
		if(now - tick > 1000)
		{
			printf("count:%d\n",count);//(uint32_t)((count*1000)/(now-tick)));
			count = 0;
			tick = now;
		}
	}	
	return 0;
}

#include "kendynet.h"
#include "kn_time.h"
#include <stdio.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"

static int count = 0;

static void redisget_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(!reply) return;
	if(reply->type != REDIS_REPLY_NIL){
		++count;
		//freeReplyObject(reply);
	}
	char buf[512];
	snprintf(buf,512,"get key%d",rand()%10000);
	kn_redisCommand(rc,buf,redisget_cb,NULL);		
	//kn_redisDisconnect(rc);
}

static void cb_redis_disconnected(redisconn_t rc){
	printf("disconnected\n");
}


static void cb_redis_connect(redisconn_t rc,int err){
	if(err == 0){
		printf("connect success\n");
		
		int i = 0;
		/*for( ; i < 100000; ++i){
			char buf[512];
			snprintf(buf,512,"set key%d %d",i+1,i+1);
			kn_redisCommand(rc,buf,NULL,NULL);
		}*/
		
		//发出1000个get
		i = 0;
		for(; i < 1000; ++i){
			char buf[512];
			snprintf(buf,512,"get key%d",rand()%10000);
			kn_redisCommand(rc,buf,redisget_cb,NULL);
		}
	}else{
		printf("connect fail:%d\n",err);
	}	
}



int main(){
	kn_proactor_t p = kn_new_proactor();
	if(0 != kn_redisAsynConnect(p,"127.0.0.1",6379,cb_redis_connect,cb_redis_disconnected)){
		printf("kn_redisAsynConnect failed\n");
		return 0;
	}
	uint32_t tick = kn_systemms();
	for(;;){
		kn_proactor_run(p,50);
		uint32_t now = kn_systemms();
		if(now - tick > 1000)
		{
			printf("count:%d\n",(uint32_t)((count*1000)/(now-tick)));
			count = 0;
			tick = now;
		}
	}	
	return 0;
}

#include "kendynet.h"
#include "kn_time.h"
#include <stdio.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"


static void redisget_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(reply->type != REDIS_REPLY_NIL){
		printf("get success\n");
	}
}


static void cb_redis_connect(redisconn_t rc,int err){
	if(err == 0){
		printf("connect success\n");
		
		//发出set 
		kn_redisCommand(rc,"set key1 1",NULL,NULL);
		//发出get
		kn_redisCommand(rc,"get key1",redisget_cb,NULL);
	}else{
		printf("connect fail:%d\n",err);
	}	
}



int main(){
	kn_proactor_t p = kn_new_proactor();
	if(0 != kn_redisAsynConnect(p,"127.0.0.1",6379,cb_redis_connect)){
		printf("kn_redisAsynConnect failed\n");
		return 0;
	}
	uint32_t tick = kn_systemms();
	for(;;){
		kn_proactor_run(p,50);
		uint32_t now = kn_systemms();
		if(now - tick > 1000)
		{
			//printf("recv:%d\n",(uint32_t)((count*1000)/(now-tick)));
			tick = now;
		}
	}	
	return 0;
}

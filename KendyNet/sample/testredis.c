#include "kendynet.h"
#include "kn_redis.h"
#include "kn_time.h"
#include "kn_timer.h"
#include <stdio.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"

static int count = 0;
static volatile int stop = 0;
engine_t p;

static void redisget_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(!reply) return;
	if(reply->type != REDIS_REPLY_NIL){
		++count;
	}else{
		printf("error:%d,%d\n",reply->type,(int)(uint64_t)pridata);
	}
	char buf[512];
	int key = rand()%10000;
	snprintf(buf,512,"get key%d",key+1);
	kn_redisCommand(rc,buf,redisget_cb,(void*)((uint64_t)key));
	//kn_redisDisconnect(rc);
}

static int c = 0;
static void redisset_cb(redisconn_t rc,redisReply *reply,void *pridata){
	if(!reply) return;
	if(++c >= 1000000) exit(0);
}


static void cb_redis_disconnected(redisconn_t rc,void *_){
	(void)_;
	printf("disconnected\n");
}

int timer_callback2(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){
		kn_redisDisconnect((redisconn_t)ud);
	}
	return 0;
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
		}
		return;*/
		//发出1000个get
		i = 0;
		for(; i < 1000; ++i){
			char buf[512];
			int key = rand()%10000;
			snprintf(buf,512,"get key%d",key+1);
			kn_redisCommand(rc,buf,redisget_cb,(void*)((uint64_t)key));
		}
		//kn_reg_timer(p,2000,timer_callback2,rc);	
	}else{
		printf("connect fail:%d\n",err);
	}	
}

int timer_callback(uint32_t event,void *ud){
	if(event == TEVENT_TIMEOUT){	
		printf("count:%d\n",count);
		count = 0;
	}
	return 0;
}

int main(){
	signal(SIGPIPE,SIG_IGN);
	p = kn_new_engine();
	if(0 != kn_redisAsynConnect(p,"127.0.0.1",6379,cb_redis_connect,cb_redis_disconnected,NULL)){
		printf("kn_redisAsynConnect failed\n");
		return 0;
	}
	kn_reg_timer(p,1000,timer_callback,NULL);	
	kn_engine_run(p);
	return 0;
}

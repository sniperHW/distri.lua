#include "kendynet.h"
#include "kn_curl.h"
#include "kn_timer.h"
#include <stdio.h>


static int count = 0;
static kn_CURLM_t curlm;
static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp){
	return size*nmemb;
}

void cb_curl(kn_CURL_t curl,CURLMsg *message,void *ud){
	++count;
	kn_curl_easy_cleanup(curl);
	//printf("here:%d,status:%d\n",count,message->data.result);
	kn_CURL_t new_curl = kn_curl_easy_init();
	kn_curl_easy_setopt(new_curl, CURLOPT_URL, "http://www.example.com/");
	kn_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);//避免输出到控制台
	kn_CURLM_add(curlm,new_curl,cb_curl,NULL);		
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
	engine_t p = kn_new_engine();
	curlm = kn_CURLM_init(p);
	int i = 0;
	for(; i < 1000; ++i){
		kn_CURL_t curl = kn_curl_easy_init();
		kn_curl_easy_setopt(curl, CURLOPT_URL, "http://www.example.com/");
		kn_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		kn_CURLM_add(curlm,curl,cb_curl,NULL);
	}
	kn_reg_timer(p,1000,timer_callback,NULL);		
	kn_engine_run(p);
	return 0;
}

#include "log.h"
#include "llist.h"
#include "msgque.h"
#include "thread.h"
#include <string.h>

static pthread_key_t  g_log_key;
static pthread_once_t g_log_key_once = PTHREAD_ONCE_INIT;
static msgque_t pending_log = NULL;//等待写入磁盘的日志项
const char *log_lev_str[] = {
	"INFO",
	"ERROR"
};


#define MAX_FILE_SIZE 1024*1024*256  //日志文件最大大小256MB

struct logfile{
	string_t filename;
	FILE    *file;
	uint32_t total_size;
};

struct log_item{
	lnode node;
	logfile_t _logfile;
	char content[0];
};

static logfile_t sys_log = NULL;

int32_t write_prefix(char *buf,uint8_t loglev)
{
	struct timespec tv;
    clock_gettime (CLOCK_REALTIME, &tv);
	struct tm _tm;
	localtime_r(&tv.tv_sec, &_tm);
	return sprintf(buf,"[%s]%04d-%02d-%02d-%02d:%02d:%02d.%03d:",log_lev_str[loglev],
				   _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec,(int32_t)tv.tv_nsec/1000000);
}

logfile_t create_logfile(const char *filename)
{
	logfile_t _logfile = calloc(1,sizeof(*_logfile));
	_logfile->filename = new_string(filename);
	return _logfile;
}


static void* log_routine(void *arg){
	while(1){
		struct log_item *item = NULL;
        msgque_get(pending_log,(lnode**)&item,100);
        if(item){
	        if(item->_logfile->file == NULL)
	        {
	        	//还没创建文件
	        	char filename[128];
	        	struct timespec tv;
	    		clock_gettime(CLOCK_REALTIME, &tv);
				struct tm _tm;
				localtime_r(&tv.tv_sec, &_tm);
				snprintf(filename,128,"%s-%04d-%02d-%02d %02d.%02d.%02d.log",to_cstr(item->_logfile->filename),
					   _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
				item->_logfile->file = fopen(filename,"w+");
				if(!item->_logfile->file){
					printf("%d\n",errno);
					free(item);	
					continue;
				}
	        }
	        fprintf(item->_logfile->file,"%s\n",item->content);
	        free(item);
	    }
	}
	return NULL;
}

static void log_once_routine(){
	pthread_key_create(&g_log_key,NULL);
	sys_log = create_logfile(SYSLOG_NAME);
	pending_log = new_msgque(64,default_item_destroyer);
	thread_run(log_routine,NULL);
}


void write_log(logfile_t logfile,const char *content)
{
	pthread_once(&g_log_key_once,log_once_routine);
	int32_t content_len = strlen(content);
	struct log_item *item = calloc(1,sizeof(*item) + content_len);
	item->_logfile = logfile;
	strncpy(item->content,content,content_len);
	int8_t ret = 0;
#ifdef MQ_HEART_BEAT
	ret = msgque_put(pending_log,(lnode*)item);
#else
	ret = msgque_put_immeda(pending_log,(lnode*)item);
#endif	
	if(ret != 0) free(item);
}

void write_sys_log(const char *content)
{
	pthread_once(&g_log_key_once,log_once_routine);
	int32_t content_len = strlen(content);
	struct log_item *item = calloc(1,sizeof(*item) + content_len);
	item->_logfile = sys_log;
	strncpy(item->content,content,content_len);
	int8_t ret = 0;
#ifdef MQ_HEART_BEAT
	ret = msgque_put(pending_log,(lnode*)item);
#else
	ret = msgque_put_immeda(pending_log,(lnode*)item);
#endif	
	if(ret != 0) free(item);
}
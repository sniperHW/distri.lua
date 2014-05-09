#include "log.h"
#include "llist.h"
#include "msgque.h"
#include "thread.h"
#include <string.h>

//static pthread_key_t  g_log_key;
static pthread_once_t g_log_key_once = PTHREAD_ONCE_INIT;
static msgque_t pending_log = NULL;//等待写入磁盘的日志项

static thread_t       g_log_thd = NULL;

static struct llist   g_log_file_list;
static mutex_t        g_mtx_log_file_list;


const char *log_lev_str[] = {
	"INFO",
	"ERROR"
};


static volatile uint8_t stop = 0;

struct logfile{
	lnode node;
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
	return sprintf(buf,"[%s]%04d-%02d-%02d-%02d:%02d:%02d.%03d[%x]:",log_lev_str[loglev],
				   _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec,
				   (int32_t)tv.tv_nsec/1000000,(uint32_t)pthread_self());
}


static void* log_routine(void *arg){
	printf("log_routine\n");
	while(1){
		uint32_t ms = stop ? 0:100;
		struct log_item *item = NULL;
        msgque_get(pending_log,(lnode**)&item,ms);
        if(item){
	        if(item->_logfile->file == NULL || item->_logfile->total_size > MAX_FILE_SIZE)
	        {
	        	if(item->_logfile->total_size){
	        		fclose(item->_logfile->file);
	        		item->_logfile->total_size = 0;
	        	}
	        	//还没创建文件
	        	char filename[128];
	        	struct timespec tv;
	    		clock_gettime(CLOCK_REALTIME, &tv);
				struct tm _tm;
				localtime_r(&tv.tv_sec, &_tm);
				snprintf(filename,128,"%s-%04d-%02d-%02d %02d.%02d.%02d.%03d.log",to_cstr(item->_logfile->filename),
					   _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec,(int32_t)tv.tv_nsec/1000000);
				item->_logfile->file = fopen(filename,"w+");
				if(!item->_logfile->file){
					printf("%d\n",errno);
					free(item);	
					continue;
				}
	        }
	        fprintf(item->_logfile->file,"%s\n",item->content);
	        //fflush(item->_logfile->file);
	        item->_logfile->total_size += strlen(item->content);
	        free(item);
	    }else if(stop){   	
	    	break;
	    }
	}
	//向所有打开的日志文件写入"log close success"
	struct logfile *l = NULL;
	char buf[128];
	mutex_lock(g_mtx_log_file_list);
	while((l = LLIST_POP(struct logfile*,&g_log_file_list)) != NULL)
	{
		int32_t size = write_prefix(buf,LOG_INFO);
        snprintf(&buf[size],128-size,"log close success");
        fprintf(l->file,"%s\n",buf);
	}	
	mutex_unlock(g_mtx_log_file_list); 	
	printf("log_routine end\n");
	return NULL;
}

static void on_process_end()
{
	printf("on_process_end\n");
	stop = 1;
	if(g_log_thd)
		thread_join(g_log_thd);
}

void _write_log(logfile_t logfile,const char *content)
{
	uint32_t content_len = strlen(content);
	struct log_item *item = calloc(1,sizeof(*item) + content_len);
	item->_logfile = logfile;
	strncpy(item->content,content,content_len);
	int8_t ret = msgque_put_immeda(pending_log,(lnode*)item);	
	if(ret != 0) free(item);
}

static void log_once_routine(){
	//pthread_key_create(&g_log_key,NULL);
	llist_init(&g_log_file_list);
	sys_log = calloc(1,sizeof(*sys_log));
	sys_log->filename = new_string(SYSLOG_NAME);
	g_mtx_log_file_list = mutex_create();
	mutex_lock(g_mtx_log_file_list);
	LLIST_PUSH_BACK(&g_log_file_list,sys_log);
	mutex_unlock(g_mtx_log_file_list);
	pending_log = new_msgque(64,default_item_destroyer);
	g_log_thd = create_thread(THREAD_JOINABLE);
	thread_start_run(g_log_thd,log_routine,NULL);
	atexit(on_process_end);
	LOG(sys_log,LOG_INFO,"log open success");
}

logfile_t create_logfile(const char *filename)
{
	pthread_once(&g_log_key_once,log_once_routine);
	logfile_t _logfile = calloc(1,sizeof(*_logfile));
	_logfile->filename = new_string(filename);
	mutex_lock(g_mtx_log_file_list);
	LLIST_PUSH_BACK(&g_log_file_list,_logfile);
	mutex_unlock(g_mtx_log_file_list);	
	LOG(_logfile,LOG_INFO,"log open success");
	return _logfile;
}


void write_log(logfile_t logfile,const char *content)
{
	_write_log(logfile,content);
}

void write_sys_log(const char *content)
{
	pthread_once(&g_log_key_once,log_once_routine);
	_write_log(sys_log,content);
}

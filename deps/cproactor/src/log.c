#include "log.h"
#include "kn_list.h"
#include "kn_thread.h"
#include <string.h>
#include "kn_channel.h"
#include "kn_proactor.h"
#include "kendynet.h"

static pthread_once_t g_log_key_once = PTHREAD_ONCE_INIT;
static kn_channel_t   pending_log;//等待写入磁盘的日志项

static kn_thread_t    g_log_thd = NULL;
static kn_proactor_t  g_log_proactor = NULL;

static kn_list        g_log_file_list;
static kn_mutex_t     g_mtx_log_file_list;


const char *log_lev_str[] = {
	"INFO",
	"ERROR"
};


static volatile uint8_t stop = 0;

struct logfile{
	kn_list_node node;
	kn_string_t filename;
	FILE    *file;
	uint32_t total_size;
};

struct log_item{
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


static void log_channel_callback(kn_channel_t c, kn_channel_t from,void *msg,void *ud)
{
	struct log_item *item = msg;
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
		snprintf(filename,128,"%s-%04d-%02d-%02d %02d.%02d.%02d.%03d.log",kn_to_cstr(item->_logfile->filename),
			   _tm.tm_year+1900,_tm.tm_mon+1,_tm.tm_mday,_tm.tm_hour,_tm.tm_min,_tm.tm_sec,(int32_t)tv.tv_nsec/1000000);
		item->_logfile->file = fopen(filename,"w+");
		if(!item->_logfile->file){
			printf("%d\n",errno);
			return;
		}
	}
	fprintf(item->_logfile->file,"%s\n",item->content);
	//fflush(item->_logfile->file);
	item->_logfile->total_size += strlen(item->content);
}	


static void* log_routine(void *arg){
	printf("log_routine\n");
	kn_channel_bind(g_log_proactor,pending_log,log_channel_callback,NULL);
	while(!stop){
		kn_proactor_run(g_log_proactor,100);	
	}
	//向所有打开的日志文件写入"log close success"
	struct logfile *l = NULL;
	char buf[128];
	kn_mutex_lock(g_mtx_log_file_list);
	while((l = (struct logfile*)kn_list_pop(&g_log_file_list)) != NULL)
	{
		int32_t size = write_prefix(buf,LOG_INFO);
        snprintf(&buf[size],128-size,"log close success");
        fprintf(l->file,"%s\n",buf);
	}	
	kn_mutex_unlock(g_mtx_log_file_list); 	
	printf("log_routine end\n");
	return NULL;
}

static void on_process_end()
{
	printf("on_process_end\n");
	stop = 1;
	if(g_log_thd)
		kn_thread_join(g_log_thd);
	kn_channel_close(pending_log);
	kn_close_proactor(g_log_proactor);
}

void _write_log(logfile_t logfile,const char *content)
{
	uint32_t content_len = strlen(content)+1;
	struct log_item *item = calloc(1,sizeof(*item) + content_len);
	item->_logfile = logfile;
	strncpy(item->content,content,content_len);
	int8_t ret = kn_channel_putmsg(pending_log,NULL,item);
	if(ret != 0) free(item);
}
			           
static void log_once_routine(){
	kn_list_init(&g_log_file_list);
	sys_log = calloc(1,sizeof(*sys_log));
	sys_log->filename = kn_new_string(SYSLOG_NAME);
	g_mtx_log_file_list = kn_mutex_create();
	kn_list_pushback(&g_log_file_list,(kn_list_node*)sys_log);
	g_log_proactor = kn_new_proactor();
	g_log_thd = kn_create_thread(THREAD_JOINABLE);
	pending_log = kn_new_channel(kn_thread_getid(g_log_thd));
	kn_thread_start_run(g_log_thd,log_routine,NULL);
	atexit(on_process_end);
	LOG(sys_log,LOG_INFO,"log open success");
}

logfile_t create_logfile(const char *filename)
{
	pthread_once(&g_log_key_once,log_once_routine);
	logfile_t _logfile = calloc(1,sizeof(*_logfile));
	_logfile->filename = kn_new_string(filename);
	kn_mutex_lock(g_mtx_log_file_list);
	kn_list_pushback(&g_log_file_list,(kn_list_node*)_logfile);
	kn_mutex_unlock(g_mtx_log_file_list);	
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

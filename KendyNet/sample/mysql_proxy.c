#include <mysql.h>
#include "kendynet.h"
#include "kn_thread.h"
#include "kn_thread_mailbox.h"
#include "stream_conn.h"
#include "kn_chr_dev.h"
#include "kn_timer.h"

#define MAX_RECONN_COUNT 10

typedef struct{
	MYSQL*              mysqlConn;
	kn_thread_t         thd;
	kn_thread_mailbox_t thd_mailbox;
	engine_t            engine;
	int                 error; 
	kn_timer_t          ping_timer;
}*mysql_conn_t;


typedef struct{
	int   len;
	char  req[0];
}*mysql_request_t;


void request_destroyer(void *ptr){
	free(ptr);
}

static int mysqlReconnect(MYSQL * hMysql)
{
	if (!mysql_ping(hMysql)) 
		return 1;
	else 
		return 0;
} 

static mysql_conn_t mysqlConn = NULL;

static void on_mail(kn_thread_mailbox_t *from,void *mail){
	mysql_request_t request = (mysql_request_t)mail;
	do{	
		int ret =mysql_real_query(mysqlConn->mysqlConn,request->req,request->len);
		if(ret == 0){
			printf("success\n");
			break;
		}else{
			ret = mysql_errno(mysqlConn->mysqlConn);
			if (ret == 2013 || ret == 2006 ||ret == 2003)
			{   // 2013CR_SERVER_LOST 2006 (CR_SERVER_GONE_ERROR)  2003 CR_CONN_HOST_ERROR 
				int count = 0;
				while (!mysqlReconnect(mysqlConn->mysqlConn)) {
					if (++count > MAX_RECONN_COUNT) 
					{
						mysqlConn->error = ret;
						kn_stop_engine(mysqlConn->engine);
						return;
					}
					kn_sleepms(1000);
				}
			} 
			else{
				printf("sql error:%d\n",ret);
				break;
			}	
		}
	}while(1);	 
}

static int timer_callback(kn_timer_t _){
	((void)_);
	int ret = mysql_ping(mysqlConn->mysqlConn);
	if(!ret){
		//记录日志
	}
	return 1;
}


void *mysql_worker(void *arg)
{
	kn_setup_mailbox(mysqlConn->engine,MODE_FAST,on_mail);
	mysqlConn->thd_mailbox = kn_self_mailbox();
	//ping every 5 min
	mysqlConn->ping_timer = kn_reg_timer(mysqlConn->engine,5*60*1000,timer_callback,NULL);
	kn_engine_run(mysqlConn->engine);
	kn_del_timer(mysqlConn->ping_timer);
	if(mysqlConn->error){
		printf("worker stop by error:%d\n",mysqlConn->error);
	}
    return NULL;
}


void chr_transfer_finish(handle_t s,st_io *io,int32_t bytestransfer,int32_t err){
	static char linebuf[4096];
	static int  index = 0;
	if(bytestransfer <= 0)
	{       
		kn_release_chrdev(s,1);
	}else{
		if(index + bytestransfer > 4096){
			index = 0;
		}else{
			//检查是否读入完整的行
			memcpy(&linebuf[index],&((char*)io->iovec[0].iov_base)[0],bytestransfer);
			int find = 0;
			int i = index;
			for(; i < index + bytestransfer; ++i){
				if(linebuf[i] == '\n'){
					find = 1;
					linebuf[i] = 0;
					break;
				}
			}
			if(find){
				printf("%s\n",linebuf);
				int len = index + i + 1;
				index = 0; 
				//读到了完整的行,发送给 mysql worker
				mysql_request_t request = calloc(1,sizeof(*request) + len);
				strcpy(request->req,linebuf);
				request->len = len;
				while(is_empty_ident(mysqlConn->thd_mailbox));
				kn_send_mail(mysqlConn->thd_mailbox,request,request_destroyer);
			}else{
				index += bytestransfer;
			}
		}		
		kn_chrdev_read(s,io);
	}
}



const char *ip = "127.0.0.1";
const int   port = 3306;
const char *usrname = "root";
const char *passwd  = "";
const char *db = "testdb";

int main(){
	MYSQL *mysql = mysql_init(NULL);
	if (!mysql)  return 0;	
	if (!mysql_real_connect(mysql,ip,usrname,passwd,db,port,NULL,0)){
		int ret = mysql_errno(mysql); 
		printf("connect mysql error host: %s user %s passwd: %s ret: %d\n",ip,usrname,passwd,ret); 
		mysql_close(mysql);
		return 0;
	}	
	mysqlConn = calloc(1,sizeof(*mysqlConn));	
	mysqlConn->mysqlConn = mysql;
    	mysqlConn->engine = kn_new_engine();
    	mysqlConn->thd = kn_create_thread(THREAD_JOINABLE);
    	kn_thread_start_run(mysqlConn->thd,mysql_worker,(void*)mysqlConn);    
	engine_t engine = kn_new_engine();
	handle_t h = kn_new_chrdev(0);
	if(!h) return 0;
	kn_chrdev_associate(h,engine,chr_transfer_finish,NULL);
	st_io io;
	io.next.next = NULL;
	struct iovec wbuf[1];
	char   buf[4096];
	wbuf[0].iov_base = buf;
	wbuf[0].iov_len = 4096;	
	io.iovec_count = 1;
	io.iovec = wbuf;
	kn_chrdev_read(h,&io);
	kn_engine_run(engine);
	return 0;
}

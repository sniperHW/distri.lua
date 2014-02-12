#ifndef _LOG_H
#define _LOG_H

#include "kn_string.h"

/*
* 一个简单的日志系统
*/

enum{
	LOG_INFO = 0,
	LOG_ERROR,
};

struct logfile;
typedef struct logfile* logfile_t;

logfile_t create_logfile(const char *filename);

void write_log(logfile_t,const char *context);

#define SYSLOG_NAME "kendynet-syslog"

//写入系统日志,默认文件名由SYSLOG_NAME定义
void write_sys_log(const char *content);

int32_t write_prefix(char *buf,uint8_t loglev);

//日志格式[INFO|ERROR]yyyy-mm-dd-hh:mm:ss.ms:content
#define LOG(LOGFILE,LOGLEV,...)\
            do{\
                char buf[4096];\
                int32_t size = write_prefix(buf,LOGLEV);\
                snprintf(&buf[size],4096-size,__VA_ARGS__);\
                write_log(LOGFILE,buf);\
            }while(0)

#define SYS_LOG(LOGLEV,...)\
            do{\
                char buf[4096];\
                int32_t size = write_prefix(buf,LOGLEV);\
                snprintf(&buf[size],4096-size,__VA_ARGS__);\
                write_sys_log(buf);\
            }while(0)             

#endif
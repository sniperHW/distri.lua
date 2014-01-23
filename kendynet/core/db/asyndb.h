#ifndef _ASYNDB_H
#define _ASYNDB_H

#include <stdint.h>
#include "llist.h"
#include "asynnet/msgdisp.h"

struct db_result;
typedef void (*DB_CALLBACK)(struct db_result*);

enum
{
	db_get = 1,
	db_set = 2
};

typedef struct db_request
{
	lnode        node;     
	DB_CALLBACK  callback;      //操作完成后的回调
	void        *ud;
	char*        query_str;     //操作请求串
	msgdisp_t    sender;        //请求者，操作结果直接返回给请求者
	uint8_t      type; 
}*db_request_t;


typedef struct db_result
{
	struct msg  base;
	DB_CALLBACK callback;
	void       *result_set;
	int32_t     err;
	void        *ud;
}*db_result_t;


typedef struct asyndb
{
	 int32_t (*connectdb)(struct asyndb*,const char *ip,int32_t port);
	 int32_t (*request)(struct asyndb*,db_request_t);
}*asyndb_t;


asyndb_t new_asyndb();
void     free_asyndb(asyndb_t);

db_result_t new_dbresult(void*,DB_CALLBACK,int32_t,void*);
void     free_dbresult(db_result_t);

db_request_t  new_dbrequest(uint8_t type,const char*,DB_CALLBACK,void*,msgdisp_t);
void     free_dbrequest(db_request_t);

void  asyndb_sendresult(msgdisp_t,db_result_t);

//db_result_t rpk_read_dbresult(rpacket_t r);


#endif
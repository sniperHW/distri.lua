#ifndef _KN_REG_FILE_H
#define _KN_REG_FILE_H
//普通文件
#include "kendynet.h"


handle_t kn_new_regfile(int fd);

//如果close非0,则同时调用close(fd)
int      kn_release_regfile(handle_t,int close);



int      kn_regfile_fd(handle_t);

int      kn_regfile_associate(handle_t,
						      engine_t,
						      void (*cb_ontranfnish)(handle_t,st_io*,int,int),
						      void (*destry_stio)(st_io*));
						   
						   
int      kn_regfile_write(handle_t,st_io*);
int      kn_regfile_read(handle_t,st_io*);	

#endif

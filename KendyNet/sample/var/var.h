#ifndef _VAR_H
#define _VAR_H

#include "kn_string.h"
#include "hash_map.h"
#include "kn_except.h"
#include "kn_exception.h"


typedef struct var* var_t;

enum{
	VAR_8 = 1,
	VAR_16,
	VAR_32,
	VAR_64,
	VAR_DOUBLE,
	VAR_STR,
	VAR_TABLE,
};



#endif
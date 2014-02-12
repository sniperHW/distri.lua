#ifndef _KN_STRING_H
#define _KN_TRING_H
#include <stdint.h>

typedef struct string* string_t;

string_t new_string(const char *);

void     release_string(string_t);

const char *to_cstr(string_t);

void     string_copy(string_t,string_t,uint32_t n);

int32_t  string_len(string_t);

void     string_append(string_t,const char*);


#endif
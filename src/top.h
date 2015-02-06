#ifndef _TOP_H
#define _TOP_H

#include "kn_string.h"

struct top;

struct top* new_top();

void destroy_top(struct top *t);

void add_filter(struct top *t,const char *filter);

kn_string_t get_top(struct top *t);

#endif
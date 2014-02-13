/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
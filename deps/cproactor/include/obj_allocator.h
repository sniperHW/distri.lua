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

#ifndef _OBJ_ALLOCATOR_H
#define _OBJ_ALLOCATOR_H

#include "kn_allocator.h"
#include "kn_list.h"
#include "kn_dlist.h"

struct obj_allocator;
typedef struct obj_allocator *obj_allocator_t;

kn_allocator_t new_obj_allocator(uint32_t objsize,uint32_t initsize);

#endif

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
#ifndef _BLOCK_OBJ_ALLOCATOR
#define _BLOCK_OBJ_ALLOCATOR

#include <stdint.h>

#define DEFAULT_BLOCK_SIZE 1024*1024

typedef struct block_obj_allocator *block_obj_allocator_t;

block_obj_allocator_t create_block_obj_allocator(uint8_t mt,uint32_t obj_size);

#endif

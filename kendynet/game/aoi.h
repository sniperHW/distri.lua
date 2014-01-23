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
#ifndef _AOI_H
#define _AOI_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dlist.h"
#define MAX_BITS 1024

struct point2D
{
	int32_t x;
	int32_t y;
};

static inline uint64_t cal_distance_2D(struct point2D *pos1,struct point2D *pos2)
{
	uint64_t tmp1 = abs(pos1->x - pos2->x);
	tmp1 *= tmp1;
	uint64_t tmp2 = abs(pos1->y - pos2->y);
	tmp2 *= tmp2;
	return (uint64_t)sqrt(tmp1 + tmp2);
}

struct bit_set
{
	uint32_t bits[MAX_BITS];
};

static inline void set_bit(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / (sizeof(uint32_t)*8);
	index %= (sizeof(uint32_t)*8);
	bs->bits[b_index] = bs->bits[b_index] | (1 << index);
}

static inline void clear_bit(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / (sizeof(uint32_t)*8);
	index %= (sizeof(uint32_t)*8);
	bs->bits[b_index] = bs->bits[b_index] & (~(1 << index));
}

static inline uint8_t is_set(struct bit_set *bs,uint32_t index)
{
	uint32_t b_index = index / (sizeof(uint32_t)*8);
	index %= (sizeof(uint32_t)*8);
	return bs->bits[b_index] & (1 << index)?1:0;
}


struct map_block
{
	struct dlist aoi_objs;
	uint32_t x;
	uint32_t y;
};

struct aoi_object
{
	struct dnode block_node;             //ͬһ��map_block�ڵĶ����γ�һ���б� 
	struct dnode super_node;     
	struct map_block *current_block;	
	uint32_t aoi_object_id; 
	struct bit_set self_view_objs;                  //�Լ��۲쵽�Ķ��󼯺�
	struct point2D current_pos;                     //��ǰ���
	uint32_t view_radius;                           //���Ӱ뾶
	uint64_t last_update_tick;
	uint32_t watch_me_count;
	uint8_t  is_leave_map;
};

#define BLOCK_LENGTH 500 //һ��Ԫ��Ĵ�СΪ5������
#define UPDATE_INTERVAL 500

typedef void (*callback_)(struct aoi_object *me,struct aoi_object *who);

struct map
{
	struct point2D top_left;            //���Ͻ�
	struct point2D bottom_right;        //���½�
	struct dlist super_aoi_objs;  //super vision objects in the map,this type of object should be Scarce
	uint32_t x_count;
	uint32_t y_count;
	callback_ enter_callback;
	callback_ leave_callback;
	struct aoi_object *all_aoi_objects[MAX_BITS*sizeof(uint32_t)*8];
	struct map_block blocks[];
};

#define STAND_RADIUS 700//��׼�Ӿ�,�Ӿ����STAND_RADIUS�Ķ���ӵ�г��Ӿ���Ҫ���⴦��

struct map *create_map(struct point2D *t_left,struct point2D *b_right,callback_ enter,callback_ leave);
void move_to(struct map *m,struct aoi_object *o,int32_t x,int32_t y);
int32_t enter_map(struct map *m,struct aoi_object *o,int32_t x,int32_t y);
int32_t leave_map(struct map *m,struct aoi_object *o);
void    tick_super_objects(struct map*m);

#endif

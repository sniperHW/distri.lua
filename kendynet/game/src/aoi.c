#include "../aoi.h"
#include "systime.h"

static inline struct map_block *get_block(struct map *m,uint32_t r,uint32_t c)
{
	return &m->blocks[r*m->x_count+c];
}

static inline void clear_map_aoi_object(struct map *m,struct aoi_object *o)
{
	printf("clear_map_aoi_object %d\n",o->aoi_object_id);
	m->all_aoi_objects[o->aoi_object_id] = NULL;
}

struct map *create_map(struct point2D *top_left,struct point2D *bottom_right,callback_ enter_callback,callback_ leave_callback)
{
	//����block�����
	uint32_t length = abs(top_left->x - bottom_right->x);
	uint32_t width = abs(top_left->y - bottom_right->y);
	
	uint32_t x_count = length % BLOCK_LENGTH == 0 ? length/BLOCK_LENGTH : length/BLOCK_LENGTH + 1;
	uint32_t y_count = width % BLOCK_LENGTH == 0 ? width/BLOCK_LENGTH : width/BLOCK_LENGTH + 1;
	
	struct map *m = calloc(1,x_count*y_count*sizeof(struct map_block)+sizeof(struct map));
	m->top_left = *top_left;
	m->bottom_right = *bottom_right;
	m->x_count = x_count;
	m->y_count = y_count;
	uint32_t x,y;
	for(y = 0; y < y_count; ++y)
		for(x = 0; x < x_count; ++x)
		{
			struct map_block * b = get_block(m,y,x);
			dlist_init(&b->aoi_objs);
			b->x = x;
			b->y = y;
		}
	dlist_init(&m->super_aoi_objs);
	m->enter_callback = enter_callback;
	m->leave_callback = leave_callback;
	return m;
}

//������������ڵĹ���Ԫ
static inline struct map_block *get_block_by_point(struct map *m,struct point2D *pos)
{	
    uint32_t x = pos->x - m->top_left.x;
	uint32_t y = pos->y - m->top_left.y;
	
	x = x/BLOCK_LENGTH;
	y = y/BLOCK_LENGTH;
	if(x >= m->x_count || y >= m->y_count)
		return NULL;
	return get_block(m,y,x);
}

//���obj����Ұ�뾶,������Ұ����صĹ���Ԫ
static inline int32_t cal_blocks(struct map *m,struct point2D *pos,uint32_t view_radius,uint32_t *x1,uint32_t *y1,uint32_t *x2,uint32_t *y2)
{

	if(get_block_by_point(m,pos) == NULL)
		return -1;

	//�����view_radius���ڵ���С����
	struct point2D top_left,bottom_right;
	top_left.x = pos->x - view_radius;
	top_left.y = pos->y - view_radius;
	bottom_right.x = pos->x + view_radius;
	bottom_right.y = pos->y + view_radius;
	
	//�������,ʹ���γ�Ϊһ���ڵ�ͼ֮�ڵĳ�����
	if(top_left.x < m->top_left.x) top_left.x = m->top_left.x;
	if(top_left.y < m->top_left.y) top_left.y = m->top_left.y;
	if(bottom_right.x > m->bottom_right.x) bottom_right.x = m->bottom_right.x;
	if(bottom_right.y > m->bottom_right.y) bottom_right.y = m->bottom_right.y;
	
	struct map_block *_top_left = get_block_by_point(m,&top_left);
	struct map_block *_bottom_right = get_block_by_point(m,&bottom_right);
	*x1 = _top_left->x;
	*y1 = _top_left->y;
	*x2 = _bottom_right->x;
	*y2 = _bottom_right->y;
	return 0;
}


static inline void enter_me(struct map *m,struct aoi_object *me,struct aoi_object *other)
{
	set_bit(&me->self_view_objs,other->aoi_object_id);
	++other->watch_me_count;
	//֪ͨme,other������Ұ
	m->enter_callback(me,other);
}

static inline void leave_me(struct map *m,struct aoi_object *me,struct aoi_object *other)
{
	clear_bit(&me->self_view_objs,other->aoi_object_id);
	if(--other->watch_me_count == 0)
		clear_map_aoi_object(m,other);
	//֪ͨme,other�뿪��Ұ
	m->leave_callback(me,other);
}

static inline void block_process_enter(struct map *m,struct map_block *bl,struct aoi_object *o)
{
	struct aoi_object *cur = (struct aoi_object*)bl->aoi_objs.head.next;
	while(cur != (struct aoi_object*)&bl->aoi_objs.tail)
	{
		uint64_t distance = cal_distance_2D(&o->current_pos,&cur->current_pos);
		if(o->view_radius >= distance)
			enter_me(m,o,cur);
		if(o!= cur && cur->view_radius >= distance)
			enter_me(m,cur,o);
		cur = (struct aoi_object *)cur->block_node.next;
	}
}

static inline void block_process_leave(struct map *m,struct map_block *bl,struct aoi_object *o,uint8_t isleave_map)
{
	struct aoi_object *cur = (struct aoi_object*)bl->aoi_objs.head.next;
	while(cur != (struct aoi_object*)&bl->aoi_objs.tail)
	{
		if(isleave_map)
		{		
			if(is_set(&cur->self_view_objs,o->aoi_object_id))
				leave_me(m,cur,o);
			if(is_set(&o->self_view_objs,cur->aoi_object_id))
				leave_me(m,o,cur);			
		}
		else
		{
			uint64_t distance = cal_distance_2D(&o->current_pos,&cur->current_pos);
			if(is_set(&o->self_view_objs,cur->aoi_object_id))
				leave_me(m,o,cur);
			if(o != cur && distance > cur->view_radius && is_set(&cur->self_view_objs,o->aoi_object_id))
				leave_me(m,cur,o);
		}
		cur = (struct aoi_object *)cur->block_node.next;	
	}
}

//��o�ƶ���new_pos,��������Ұ�仯
void move_to(struct map *m,struct aoi_object *o,int32_t _x,int32_t _y)
{
	struct point2D new_pos = {_x,_y};
	struct point2D old_pos = o->current_pos;
	o->current_pos = new_pos;
	struct map_block *old_block = get_block_by_point(m,&old_pos);
	struct map_block *new_block = get_block_by_point(m,&new_pos);
	if(old_block != new_block)
		dlist_remove(&o->block_node);
		
	uint32_t radius = STAND_RADIUS;	
	if(o->view_radius > STAND_RADIUS)
		radius = o->view_radius;		
	//�����¾ɹ�������
	uint32_t n_x1,n_y1,n_x2,n_y2;
	uint32_t o_x1,o_y1,o_x2,o_y2;
	n_x1 = n_y1 = n_x2 = n_y2 = 0;
	o_x1 = o_y1 = o_x2 = o_y2 = 0;
	cal_blocks(m,&old_pos,radius,&o_x1,&o_y1,&o_x2,&o_y2);
	cal_blocks(m,&new_pos,radius,&n_x1,&n_y1,&n_x2,&n_y2);
	
	uint32_t y = n_y1;
	uint32_t x = 0;
	for( ; y <= n_y2; ++y)
	{
		for( x = n_x1; x <= n_x2; ++x)
		{
			if(x >= o_x1 && x <= o_x2 && y >= o_y1 && y <= o_y2)
			{
				//�ޱ仯����
				struct map_block *bl = get_block(m,y,x);
				struct aoi_object *cur = (struct aoi_object*)bl->aoi_objs.head.next;
				while(cur != (struct aoi_object*)&bl->aoi_objs.tail)
				{
					uint64_t distance = cal_distance_2D(&new_pos,&cur->current_pos);
					if(o != cur)
					{
						if(o->view_radius >= distance && !is_set(&o->self_view_objs,cur->aoi_object_id))
							enter_me(m,o,cur);
						else if(o->view_radius < distance && is_set(&o->self_view_objs,cur->aoi_object_id))
							leave_me(m,o,cur);
						
						if(cur->view_radius >= distance && !is_set(&cur->self_view_objs,o->aoi_object_id))
							enter_me(m,cur,o);
						else if(cur->view_radius < distance && is_set(&cur->self_view_objs,o->aoi_object_id))
							leave_me(m,cur,o);
					}
					cur = (struct aoi_object *)cur->block_node.next;			
				}				
			}
			else
			{
				//�½��������
				block_process_enter(m,get_block(m,y,x),o);
			}
		}
	}
	if(old_block != new_block)
		dlist_push(&new_block->aoi_objs,&o->block_node);
	y = o_y1;
	for( ; y <= o_y2; ++y)
	{
		for(x = o_x1; x <= o_x2; ++x)
		{
			if(x >= n_x1 && x <= n_x2 && y >= n_y1 && y <= n_y2)
				continue;//���ﲻ�����ޱ仯����
			block_process_leave(m,get_block(m,y,x),o,0);
		}		
	}
	o->last_update_tick = GetSystemMs64();
}


int32_t enter_map(struct map *m,struct aoi_object *o,int32_t _x,int32_t _y)
{
	o->current_pos.x = _x;
	o->current_pos.y = _y;
	struct map_block *block = get_block_by_point(m,&o->current_pos);
	if(!block)
		return -1;
	dlist_push(&block->aoi_objs,&o->block_node);
	m->all_aoi_objects[o->aoi_object_id] = o;
	uint32_t radius = STAND_RADIUS;	
	if(o->view_radius > STAND_RADIUS)
	{
		radius = o->view_radius;
		dlist_push(&m->super_aoi_objs,&o->super_node);
	}
	uint32_t x1,y1,x2,y2;
	x1 = y1 = x2 = y2 = 0;
	cal_blocks(m,&o->current_pos,radius,&x1,&y1,&x2,&y2);
	uint32_t y = y1;
	uint32_t x;
	for( ; y <= y2; ++y)
	{
		for(x=x1 ; x <= x2; ++x)
		{
			block_process_enter(m,get_block(m,y,x),o);
		}		
	}
	o->last_update_tick = GetSystemMs64();
	o->is_leave_map = 0;
	return 0;
}

int32_t leave_map(struct map *m,struct aoi_object *o)
{
	struct map_block *block = get_block_by_point(m,&o->current_pos);
	if(!block)
		return -1;
	dlist_remove(&o->block_node);
	uint32_t radius = STAND_RADIUS;	
	if(o->view_radius > STAND_RADIUS)
	{
		radius = o->view_radius;
		dlist_remove(&o->super_node);
	}	
	uint32_t x1,y1,x2,y2;
	x1 = y1 = x2 = y2 = 0;
	cal_blocks(m,&o->current_pos,radius,&x1,&y1,&x2,&y2);
	uint32_t y = y1;
	uint32_t x;
	for( ; y <= y2; ++y)
	{
		for( x=x1; x <= x2; ++x)
		{
			block_process_leave(m,get_block(m,y,x),o,1);
		}		
	}
	o->is_leave_map = 1;

	//自己离开自己的视野
	leave_me(m,o,o);
	return 0;			
}

static inline void tick_super_object(struct map *m,struct aoi_object *o)
{
	uint32_t now = GetSystemMs();
	if(now - o->last_update_tick >= UPDATE_INTERVAL)
	{ 
		//remove out of view object first
		uint32_t i = 0;
		for( ; i < MAX_BITS; ++i)
		{
			if(o->self_view_objs.bits[i] > 0)
			{
				uint32_t j = 0;
				for( ; j < sizeof(uint32_t); ++j)
				{
					if(o->self_view_objs.bits[i] & (1 << j))
					{
						uint32_t aoi_object_id = i*sizeof(uint32_t) + j;
						if(aoi_object_id != o->aoi_object_id)
						{
							struct aoi_object *other = m->all_aoi_objects[aoi_object_id];
							if(other->is_leave_map)
								leave_me(m,o,other);
							else
							{
								uint64_t distance = cal_distance_2D(&o->current_pos,&other->current_pos);
								if(distance > o->view_radius)
									leave_me(m,o,other);
							}
						}
					}
				}
			}
		}
		//process enter view
		uint32_t x1,y1,x2,y2;
		x1 = y1 = x2 = y2 = 0;
		cal_blocks(m,&o->current_pos,o->view_radius,&x1,&y1,&x2,&y2);
		uint32_t y = y1;
		uint32_t x;
		for( ; y <= y2; ++y)
		{
			for( x=x1; x <= x2; ++x)
			{
				struct map_block *bl = get_block(m,y,x);
				struct aoi_object *cur = (struct aoi_object*)bl->aoi_objs.head.next;
				while(cur != (struct aoi_object*)&bl->aoi_objs.tail)
				{
					if(is_set(&o->self_view_objs,cur->aoi_object_id) == 0)
					{
						uint64_t distance = cal_distance_2D(&o->current_pos,&cur->current_pos);
						if(o->view_radius >= distance)
							enter_me(m,o,cur);
					}
					cur = (struct aoi_object *)cur->block_node.next;
				}
			}		
		}		
		o->last_update_tick = now;	
	}
	
}

void tick_super_objects(struct map *m)
{
	struct dnode *cur = m->super_aoi_objs.head.next;
	while(cur != &m->super_aoi_objs.tail)
	{
		struct aoi_object *o = (struct aoi_object*)((uint8_t*)cur - sizeof(struct dnode));
		tick_super_object(m,o);
		cur = cur->next;
	}
}

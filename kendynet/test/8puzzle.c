/*8码问题测试用例*/
#include "game/astar.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "dlist.h"
#include "common_hash_function.h"
#include <stdlib.h>
#include <string.h>
#include "systime.h"
struct _8puzzle_map;
struct _8puzzle_node
{
	struct map_node _base;
	lnode _lnode;
	struct _8puzzle_map *_map;
	int puzzle[3][3];
	int zero_x;
	int zero_y;
};

int direction[4][2] = {
	{0,-1},//上
	{0,1},//下
	{-1,0},//左
	{1,0},//右
};

static inline int32_t _hash_key_eq_(void *l,void *r)
{
	if(*(uint64_t*)r == *(uint64_t*)l)
		return 0;
	return -1;
}

static inline uint64_t _hash_func_(void* key)
{
	return burtle_hash(key,sizeof(uint64_t),1);
}

static inline uint64_t puzzle_value(int p[3][3])
{
	uint64_t pv = 0;
	static int factor[9] = {
		100000000,
		10000000,
		1000000,
		100000,
		10000,
		1000,
		100,
		10,
		1,		
	};
	int c = 0;
	int i = 0;
	int j = 0;
	for( ; i < 3; ++i)
		for(j = 0;j < 3;++j)
			pv += p[i][j]*factor[c++];
	return pv;
}

struct _8puzzle_map
{	
	struct llist mnodes;
	hash_map_t puzzle_2_mnode;
};

struct _8puzzle_node* getnode_by_pv(struct _8puzzle_map *pmap,int p[3][3],int zero_x,int zero_y)
{
	hash_map_t h = pmap->puzzle_2_mnode;
	uint64_t pv = puzzle_value(p);
	struct _8puzzle_node* pnode = NULL;
	hash_map_iter it = HASH_MAP_FIND(uint64_t,h,pv);
	hash_map_iter end = hash_map_end(h);
	if(!IT_EQ(it,end))
		pnode = (struct _8puzzle_node*)IT_GET_VAL(void*,it);
	else
	{
		pnode = calloc(1,sizeof(*pnode));
		memcpy(pnode->puzzle,p,sizeof(int)*9);
		pnode->_map = pmap;
		LLIST_PUSH_BACK(&pmap->mnodes,&pnode->_lnode);
		HASH_MAP_INSERT(uint64_t,void*,h,pv,(void*)pnode);
		if(zero_x >= 0 && zero_y >= 0)
		{
			pnode->zero_x = zero_x;
			pnode->zero_y = zero_y;
			return pnode;
		}
		int i = 0;
		int j = 0;
		for( ; i < 3; ++i)
		{
			for(j = 0; j < 3; ++j)
				if(p[i][j] == 0)
				{
					pnode->zero_x = j;
					pnode->zero_y = i;
					return pnode;
				}
		}

	}
	return pnode;
}

static inline void swap(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

struct map_node** _8_get_neighbors(struct map_node *mnode)
{
	struct map_node **ret = (struct map_node **)calloc(5,sizeof(*ret));
	struct _8puzzle_node *__8puzzle_node = (struct _8puzzle_node*)mnode;
	struct _8puzzle_map *_puzzle_map = __8puzzle_node->_map;
	int p[3][3];
	int32_t i = 0;
	int32_t c = 0;
	for( ; i < 4; ++i)
	{
		int x = __8puzzle_node->zero_x + direction[i][0];
		int y = __8puzzle_node->zero_y + direction[i][1];
		if(x < 0 || x >=3 || y < 0 || y >= 3)
			continue;
		memcpy(p,__8puzzle_node->puzzle,sizeof(int)*9);
		swap(&p[y][x],&p[__8puzzle_node->zero_y][__8puzzle_node->zero_x]);		
		struct _8puzzle_node *tmp = getnode_by_pv(_puzzle_map,p,x,y);
		ret[c++] = (struct map_node*)tmp;
	}
	ret[c] = NULL;
	return ret;
}

double _8_cost_2_neighbor(struct path_node *from,struct path_node *to)
{
	return 1.0f;
} 
double _8_cost_2_goal(struct path_node *from,struct path_node *to)
{
	struct _8puzzle_node *_from = (struct _8puzzle_node*)from->_map_node;
	struct _8puzzle_node *_to = (struct _8puzzle_node*)to->_map_node;
	int *tmp_from = (int*)_from->puzzle;
	int *tmp_to = (int*)_to->puzzle;
	int i = 0;
	double sum = 0.0f;
	for( ; i < 9; ++i)
		if(tmp_from[i] != tmp_to[i])
			++sum;	
	return sum*4.66f;
}

struct _8puzzle_map *create_map()
{
	struct _8puzzle_map *map = (struct _8puzzle_map *)calloc(1,sizeof(*map));
	llist_init(&map->mnodes);
	map->puzzle_2_mnode = hash_map_create(40960,sizeof(uint64_t),sizeof(void*),_hash_func_,_hash_key_eq_);
	return map;
}

void destroy_map(struct _8puzzle_map **map)
{
	lnode *n = NULL;
	while((n = llist_pop(&(*map)->mnodes))!=NULL)
	{
		struct _8puzzle_node *tmp = (struct _8puzzle_node*)((char*)n-sizeof(struct map_node));
		free(tmp);
	}
	free(*map);
}

int8_t check(int *a,int *b)
{
	uint32_t _a = 0;
	uint32_t _b = 0;
	int i = 0;
	int j = 0;
	for( ; i < 9; ++i)
	{
		for( j = i+1;j<9;++j)
			if((a[i]!=0 && a[j]) != 0 && (a[i] > a[j]))
				++_a;
	}
	for( i=0; i < 9; ++i)
	{
		for( j = i+1;j<9;++j)
			if((b[i]!=0 && b[j]) != 0 && (b[i] > b[j]))
				++_b;
	}
	return _a%2 == _b%2;
}


void show_8puzzle(struct _8puzzle_map *map,struct A_star_procedure *astar,int _from[3][3],int _to[3][3])
{
	if(!check(&_from[0][0],&_to[0][0]))
	{
		printf("no way\n");
	}	
	int path_count = 0;
	uint64_t tick = GetSystemMs64();
	struct map_node *from = (struct map_node*)getnode_by_pv(map,_from,-1,-1);
	struct map_node *to = (struct map_node*)getnode_by_pv(map,_to,-1,-1);
	struct path_node *path = find_path(astar,from,to);
	tick = GetSystemMs64() - tick;
	if(!path)
		printf("no way\n");
	while(path)
	{
		struct _8puzzle_node *mnode = (struct _8puzzle_node *)path->_map_node;
		int i = 0;
		int j = 0;
		for( ; i < 3; ++i)
		{
			for(j = 0; j < 3; ++j)
			{
				if(mnode->puzzle[i][j] == 0)
					printf(" ");
				else
					printf("%d",mnode->puzzle[i][j]);
			}
			printf("\n");
		}
		printf("\n");
		path = path->parent;
		++path_count;
	}
	printf("path_count:%d,%d\n",path_count,tick);
}

int main()
{
	int f0[3][3] = {
		{2,3,4},
		{1,8,5},
		{0,7,6},
	};
	int t0[3][3] = {
		{1,2,3},
		{8,0,4},
		{7,6,5},
	};
	int f1[3][3] = {
		{1,2,3},
		{4,5,6},
		{7,8,0},
	};
	int t1[3][3] = {
		{8,7,6},
		{5,4,3},
		{2,1,0},
	};			
	struct _8puzzle_map *map = create_map();
	struct A_star_procedure *astar = create_astar(_8_get_neighbors,_8_cost_2_neighbor,_8_cost_2_goal);
	show_8puzzle(map,astar,f0,t0);
	show_8puzzle(map,astar,f1,t1);
	destroy_map(&map);
	destroy_Astar(&astar);	
	return 0;
}




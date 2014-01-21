/*A*迷宫测试用例*/
#include "game/astar.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
struct maze_map;
struct maze_node
{
	struct map_node _base;
	struct maze_map *_map;
	int value;
	/*在地图中的x,y坐标*/
	int x; 
	int y;	
};

#define BLOCK 2//阻挡

int direction[8][2] = {
	{0,-1},//上
	{0,1},//下
	{-1,0},//左
	{1,0},//右
	{-1,-1},//左上
	{1,-1},//右上
	{-1,1},//左下
	{1,1},//右下
};

struct maze_map
{
	struct maze_node **_maze_map;
	int    max_x;
	int    max_y;
};

struct maze_node *get_mazenode_by_xy(struct maze_map *map,int x,int y)
{
	if(x < 0 || x >= map->max_x || y < 0 || y >= map->max_y)
		return NULL;
	return map->_maze_map[y*map->max_x+x];
}

//获得当前maze_node的8个临近节点,如果是阻挡点会被忽略
struct map_node** maze_get_neighbors(struct map_node *mnode)
{
	struct map_node **ret = (struct map_node **)calloc(9,sizeof(*ret));
	struct maze_node *_maze_node = (struct maze_node*)mnode;
	struct maze_map *_maze_map = _maze_node->_map;
	int32_t i = 0;
	int32_t c = 0;
	for( ; i < 8; ++i)
	{
		int x = _maze_node->x + direction[i][0];
		int y = _maze_node->y + direction[i][1];
		struct maze_node *tmp = get_mazenode_by_xy(_maze_map,x,y);
		if(tmp && tmp->value != BLOCK)
			ret[c++] = (struct map_node*)tmp;
	}
	ret[c] = NULL;
	return ret;
}

//计算到达相临节点需要的代价
double maze_cost_2_neighbor(struct path_node *from,struct path_node *to)
{
	struct maze_node *_from = (struct maze_node*)from->_map_node;
	struct maze_node *_to = (struct maze_node*)to->_map_node;
	int delta_x = _from->x - _to->x;
	int delta_y = _from->y - _to->y;
	int i = 0;
	for( ; i < 8; ++i)
	{
		if(direction[i][0] == delta_x && direction[i][1] == delta_y)
			break;
	}
	if(i < 4)
		return 50.0f;
	else if(i < 8)
		return 60.0f;
	else
	{
		assert(0);
		return 0.0f;
	}	
}

double maze_cost_2_goal(struct path_node *from,struct path_node *to)
{
	struct maze_node *_from = (struct maze_node*)from->_map_node;
	struct maze_node *_to = (struct maze_node*)to->_map_node;
	int delta_x = abs(_from->x - _to->x);
	int delta_y = abs(_from->y - _to->y);
	return (delta_x * 50.0f) + (delta_y * 50.0f);
}

struct maze_map* create_map(int *array,int max_x,int max_y)
{
	struct maze_map *_map = calloc(1,sizeof(*_map));
	_map->max_x = max_x;
	_map->max_y = max_y;
	_map->_maze_map = (struct maze_node**)calloc(max_x*max_y,sizeof(struct maze_node*));
	int i = 0;
	int j = 0;
	for( ; i < max_y; ++i)
	{
		for(j = 0; j < max_x;++j)
		{		
			_map->_maze_map[i*max_x+j] = calloc(1,sizeof(struct maze_node));
			struct maze_node *tmp = _map->_maze_map[i*max_x+j];
			tmp->_map = _map;
			tmp->x = j;
			tmp->y = i;
			tmp->value = array[i*max_x+j];			
		}
	}
	return _map;
}

void destroy_map(struct maze_map **map)
{
	free((*map)->_maze_map);
	free(*map);
}

void show_maze(struct maze_map *map,struct A_star_procedure *astar,int fx,int fy,int tx,int ty)
{
	struct map_node *from = (struct map_node*)get_mazenode_by_xy(map,fx,fy);
	struct map_node *to = (struct map_node*)get_mazenode_by_xy(map,tx,ty);
	struct path_node *path = find_path(astar,from,to);
	printf("\n");
	if(!path)
		printf("no way\n");
	while(path)
	{
		struct maze_node *mnode = (struct maze_node *)path->_map_node;
		mnode->value = 1;
		path = path->parent;
	}	
	//重新打印地图和路径
	int i = 0;
	int j = 0;
	for(; i < map->max_y; ++i)
	{
		for(j = 0; j < map->max_x; ++j)
		{
			struct maze_node *tmp = get_mazenode_by_xy(map,j,i);
			if(tmp->value != 0)
			{
				printf("%d",tmp->value);
				if(tmp->value == 1)
					tmp->value = 0;
			}
			else
				printf(" ");
		}
		printf("\n");
	}
}

int main()
{
	int array[15][25] =
	{
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
		{2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
		{2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
		{2,0,0,2,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,2,2,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,0,0,2,0,0,2,0,0,2,0,0,0,0,0,0,2,0,0,0,0,2},
		{2,0,0,2,2,2,2,0,0,2,2,2,2,0,0,0,0,0,0,2,2,2,2,0,2},
		{2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
		{2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2},
		{2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
	};
	struct maze_map *map = create_map((int*)array,25,15);
	struct A_star_procedure *astar = create_astar(maze_get_neighbors,maze_cost_2_neighbor,maze_cost_2_goal);
	show_maze(map,astar,1,1,4,10);
	show_maze(map,astar,1,1,11,10);
	show_maze(map,astar,1,1,4,1);	
	destroy_map(&map);
	destroy_Astar(&astar);
	return 0;
}

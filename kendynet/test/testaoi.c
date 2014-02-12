#include <stdio.h>
#include "game/aoi.h"
#include "core/kn_string.h"

struct player
{
	struct aoi_object aoi;
	string_t name; 
};


void enter_me(struct aoi_object *me,struct aoi_object *who)
{
	struct player* l_me = (struct player*)me;
	struct player* l_who = (struct player*)who;

	printf("%s enter %s\n",to_cstr(l_me->name),to_cstr(l_who->name));

}

void leave_me(struct aoi_object *me,struct aoi_object *who)
{
	struct player* l_me = (struct player*)me;
	struct player* l_who = (struct player*)who;

	printf("%s leave %s\n",to_cstr(l_me->name),to_cstr(l_who->name));	
}


int main(){

	struct player *ply1 = calloc(1,sizeof(*ply1));
	struct player *ply2 = calloc(1,sizeof(*ply2));
	struct player *ply3 = calloc(1,sizeof(*ply3));
	ply1->name = new_string("a");
	ply2->name = new_string("b");
	ply3->name = new_string("c");

	ply1->aoi.aoi_object_id = 0;
	ply2->aoi.aoi_object_id = 1;
	ply3->aoi.aoi_object_id = 2;

	struct point2D t_left,b_right;
	t_left.x = 0;
	t_left.y = 0;
	b_right.x = 150;
	b_right.y = 150;

	struct map *l_map = create_map(&t_left,&b_right,enter_me,leave_me);

	//a进入地图,坐标10,10
	enter_map(l_map,&ply1->aoi,10,10);

	//b进入地图,坐标60,10
	enter_map(l_map,&ply2->aoi,60,10);

	//C进入地图,坐标75,75
	enter_map(l_map,&ply3->aoi,75,75);

	printf("begin move\n");

	move_to(l_map,&ply3->aoi,60,10);

	move_to(l_map,&ply3->aoi,10,10);

	printf("c leave map\n");

	leave_map(l_map,&ply3->aoi);
	printf("a leave map\n");
	leave_map(l_map,&ply1->aoi);
	printf("b leave map\n");
	leave_map(l_map,&ply2->aoi);

	return 0;
}
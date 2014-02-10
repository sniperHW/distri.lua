#include "superservice.h"


void load_ply_info_cb(struct db_result *result)
{
	//数据导入后的回调函数
	player_t ply = result->ud;
	if(result->err == 0){
		//成功，通知gate
	}else{
		//失败,通知gate

		//释放player
	}
}

static void load_player_info(player_t ply);

void player_login(rpacket_t rpk)
{
	string_t actname = rpk_read_string(rpk);
	uint32_t gateident = rpk_read_uint32(rpk);
	player_t ply = find_player_by_actname(actname);
	if(ply){
		//对象还未销毁，重新绑定关系

	}else{
		ply = create_player(actname,gateident);
		if(!ply){
			//通知玩家系统繁忙
			return;
		}
		load_player_info(ply);//从数据库导入玩家信息
	}
}


//玩家请求登出
void player_logout(rpacket_t rpk)
{

}
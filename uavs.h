/*******************************
@@Author     : Charles
@@Date       : 2018-05-10
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#ifndef __UAVS_H__
#define __UAVS_H__ 

#include "json.h"
#include "config.h"

#define UAV_STAT_NORMAL 	0
#define UAV_STAT_DESTROYED 	1
#define UAV_STAT_IN_FOG 	2
#define UAV_STAT_CHARGING 	3

#define GOOD_STAT_NORMAL 	 0
#define GOOD_STAT_CARRIED 	 1
#define GOOD_STAT_DISAPPEAR -1
//无人机的价格表
struct _uav_para
{
	char uav_type[5];
	short load_weight;
	short value;
	int capacity;
	int charge;
};
//无人机路径的链表表示！！
struct _node
{
	short cdnt_xyz[3]; 
	struct _node* next; //下一个坐标点
};
typedef struct _node* CDNT_NODE;
typedef struct _node* ROUTE_LIST;
//真正的每个无人机信息结构体！
struct _uav
{
	struct _good* aim_good;//NULL表示没有目标或者携带
	short aim_xyz[3];
	short curr_xyz[3];
	ROUTE_LIST route;
	short good_id;  //携带的货物id
	int remain_electricity;
	short status; //0-normal  1-destroyed  2-in_fog
	short wait_time;
	struct _uav_para* uav_para;
};
//good结构体
struct _good {
	short id;
	short start_xyz[3];
	short end_xyz[3];
	int weight;
	int value;
	int start_time;
	int remain_time;
	int left_time;
	char aimed_uavs[UAV_MAX_NUM]; // 0-没被锁定  1-被锁定
	int aimed_uav_id;
	short aimed_uavs_num;
	int status; // 0-normal  1-be_carried -1-no_exist
	ROUTE_LIST transport_route;
	int transport_steps_num;
};

#define IsUAVFree(u) (!(u.status != UAV_STAT_DESTROYED && u.aim_good == NULL && u.good_id == -1))
#define IsUAVStandby(u) (!(!IsUAVFree(u) && u.route->next == NULL))
#define IsUAVNeedPlan(u) (!(u.status != UAV_STAT_DESTROYED && u.route->next == NULL && (u.curr_xyz[0] != u.aim_xyz[0] || u.curr_xyz[1] != u.aim_xyz[1] || u.curr_xyz[2] != u.aim_xyz[2])))

#define IsGoodFree(g) (!(!g.aimed_uavs_num && g.status == GOOD_STAT_NORMAL))

int UAVsParaInit(json_object* json_obj, struct _game_para* game_paras_tmp);
int UpdateUAVsInfo(struct _game_para* game_paras, json_object* json_obj_uav, struct _uav* uavs);
int UpdateGoodsInfo(struct _game_para*, json_object* json_obj, struct _good*);
int FreeUAVsInfo(struct _uav* u);
int FreeGoodsInfo(struct _good* goods);
int CountUAVsNum(struct _good* const g);
unsigned int CalculateSuitability(struct _good* const g, struct _uav* const u);
int BubbleSortGoods(struct _good* , struct _game_para*, int size);
int BubbleSortParas(struct _game_para* game_paras_tmp, struct _uav_para* para, int size);

#endif
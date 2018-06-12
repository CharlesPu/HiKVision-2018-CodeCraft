/*******************************
@@Author     : Charles
@@Date       : 2018-05-07
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#ifndef __CONFIG_H__
#define __CONFIG_H__

/************************调试相关*************************/
// #define PRINT_SEND
// #define PRINT_RECV
// #define PRINT_UAVS_PARAS
// #define PRINT_DISTRIBUTION

// #define PRINT_BFS
// #define PRINT_BATTLE_TIME_NOTICE
// #define PRINT_COST_TIME
// #define PRINT_TOTAL_VALUE
// #define PRINT_WAIT_CAUSE
// #define PRINT_CHARGE_NUM

/******************socket相关*********************/
//我测试的服务器
// #define SERV_IP "111.231.89.253"
// #define SERV_PORT 40000
// #define TOKEN "ABCDEFG"
#define BUF_LENGTH 7168 //client能接收的最大长度

/**************************UAV相关***************************/
#define UAV_MAX_NUM 512  //整场比赛可能有的我方最大无人机数
#define UAV_TYPE_MAX_NUM 256
#define UAV_MAX 85 //我方允许拥有的最大有效uav数
//UAV价值等级换算基数
#define UAV_VAL_CVT_BASE 40 //结合map地图，最大能表示价值2440左右的uav的等级
// UAV和good的匹配度换算最大范围
#define STBLT_CVT_RANGE 1000080000 //(1000^3 + 80000)
#define UAV_MAX_WAIT_STEPS_NUM 1 //UAV最大等待步数
//uav最大步长！！核心！！！
#define UAV_STEP_MAX_LENGTH 400//400 //300 //200  //semi:500
#define UAV_STEP_MAX_LENGTH_SQRT 20//20 //16 //12  //semi:22
//待命区半径和矩形比例
#define STANDBY_RADIUS_SCALE 2 //即半径为长或者宽的1/2
#define UAV_BACK_CHARGE_MAX_NUM 5 //回家充电的uav最大数量
#define UAV_ATTACK_MAX_NUM 0 //攻击机最大数
#define UAV_ATTACK_DISTRIBUTE_INTERVAL 8 //战场上没有good多久之后才开始分配攻击机
#define UAV_BACK_DISTRIBUTE_INTERVAL   8 //战场上没有good多久之后才开始back home

/*************************good相关***************************/
#define GOOD_MAX_NUM 256
#define GOOD_TRANS_MARGIN 4 //分配时考虑的电量的余度
// #define GOOD_BFS_IMPROVED 

/****************************图相关******************************/
#define GRAPH_MAX_NUM 4 //比赛中允许的最大走的层数
#define GRAPH_GOOD_MAX_NUM 1 //good的bfs的最大层数，，，
#define GRAPH_VEX_MAX_NUM 36000 //比赛中允许的最大点数...
#define GRAPH_GOOD_BFS_LOOP_MAX 2
#define GRAPH_UAV_BFS_LOOP_MAX 4

/*******************************map相关*******************************/
#define MAP_CHARGE_WAIT_DIS_POW 100 // 回家充电的话如果家里有人要出来，在多远处停住 10^2
#define MAP_STAT_JUDGE_DIS_POW 2 // 家里状态判断的范围 sqrt(2)^2

struct _game_para {
	char user_id[200];
	char room_id[200];
	char serv_ip[20];
	int serv_port;
	char token[200];

	unsigned int battle_time;
	unsigned int we_value;
	unsigned int enemy_value;

	unsigned int uavs_num;//比赛中我方所有无人机的总数！
	unsigned int uavs_real_num;//我方拥有的有效uav数
	unsigned int goods_num;//比赛中出现过的所有good总数
	int goods_real_num;//此时能锁定的good总数
	unsigned int uavs_type_num;//比赛中能够买的uav种类数
	short goods_val_sort[GOOD_MAX_NUM];//从大到小
	short uavs_val_sort[UAV_TYPE_MAX_NUM];//uav_para value从小到大
	short no_good_interval;//没有货物的间隔时间
	short is_attack_uav[UAV_MAX_NUM];//某uav是不是撞击uav
	short back_charge_uavs_num;//回家充电的uav数量
};

#endif
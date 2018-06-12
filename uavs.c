/*******************************
@@Author     : Charles
@@Date       : 2018-05-10
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "uavs.h"
#include "json.h"
#include "config.h"
#include "route.h"

//全局变量！！
struct _uav_para* UAVs_para;//以type数作为索引
short* UAVs_para_id;
extern short we_parking[2];
extern unsigned short low_limit;
/*************************************************
@Description: 各种uav价格等属性表
@Input: 服务器开始给的json的map键的值  
@Output: 
@Return: 0-成功   1-失败  
@Others: 改变了全局变量 UAVs_para 结构体数组
*************************************************/
int UAVsParaInit(json_object* json_obj_map, struct _game_para* game_paras_tmp)
{
	json_object* json_obj_map_tmp = json_obj_map;
	json_object* json_val = json_object_object_get(json_obj_map_tmp, "UAV_price");
	//初始化更新全局变量 UAVs_para 结构体数组
	int uavs_type_num = json_object_array_length(json_val);
	UAVs_para = (struct _uav_para*)malloc(sizeof(struct _uav_para) * UAV_TYPE_MAX_NUM);
	for (int i = 0; i < uavs_type_num; ++i)
	{
		memset(UAVs_para[i].uav_type, 0, sizeof(UAVs_para[i].uav_type));
		UAVs_para[i].load_weight = 0;
		UAVs_para[i].value = 0;
	}
	game_paras_tmp->uavs_type_num = uavs_type_num;
	UAVs_para_id = (short*)malloc(sizeof(short) * uavs_type_num);
#ifdef PRINT_UAVS_PARAS
	printf("uav paras in this battle:\n");
#endif	
	//解析uav的价格等参数
	for (int i = 0; i < uavs_type_num; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_val, i);
		char type[6] = {0}; 
		memcpy(type, json_object_to_json_string(json_object_object_get(json_arr_tmp, "type")), 
			strlen(json_object_to_json_string(json_object_object_get(json_arr_tmp, "type"))));
		//attention!!! from 2, because "F1", the exist of "
		int idx = atoi(type + 2);
		UAVs_para_id[i] = idx;
		memcpy(UAVs_para[idx].uav_type, type + 1, strlen(type) - 2);
		UAVs_para[idx].load_weight = json_object_get_int(json_object_object_get(json_arr_tmp, "load_weight"));
		UAVs_para[idx].capacity = json_object_get_int(json_object_object_get(json_arr_tmp, "capacity"));
		UAVs_para[idx].charge = json_object_get_int(json_object_object_get(json_arr_tmp, "charge"));
		UAVs_para[idx].value = json_object_get_int(json_object_object_get(json_arr_tmp, "value"));
#ifdef PRINT_UAVS_PARAS
		printf("uav_type %s :lw %d, val %d, cap: %d, chag %d\n", type, UAVs_para[idx].load_weight, UAVs_para[idx].value, UAVs_para[idx].capacity, UAVs_para[idx].charge);
#endif
	}	
	BubbleSortParas(game_paras_tmp, UAVs_para, game_paras_tmp->uavs_type_num);
	//start from 1!!!!!
#ifdef PRINT_UAVS_PARAS
	for (int i = 1; i < (game_paras_tmp->uavs_type_num + 1); ++i)
		printf("%d:%d\n", i,game_paras_tmp->uavs_val_sort[i]);
#endif	
	json_obj_map_tmp = NULL;
	return 0;
}

/*************************************************
@Description: 更新每个uav结构体的信息！
@Input: 服务器给的uav json数组   所有的uav结构体数组
@Output: 所有的uav结构体数组
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int UpdateUAVsInfo(struct _game_para* game_paras, json_object* json_obj_uav, struct _uav* uavs)
{
	struct _game_para* game_paras_tmp = game_paras;
	struct _uav* uavs_tmp = uavs;
	json_object* json_obj_uav_tmp = json_obj_uav;
	int uavs_num_tmp = json_object_array_length(json_obj_uav_tmp);
	game_paras_tmp->uavs_num = uavs_num_tmp;
	
	game_paras_tmp->uavs_real_num = 0;

	for (int i = 0; i < uavs_num_tmp; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_obj_uav_tmp, i);
		int uav_no = json_object_get_int(json_object_object_get(json_arr_tmp, "no"));
		short cdnt_x = json_object_get_int(json_object_object_get(json_arr_tmp, "x"));
		short cdnt_y = json_object_get_int(json_object_object_get(json_arr_tmp, "y"));
		short cdnt_z = json_object_get_int(json_object_object_get(json_arr_tmp, "z"));
		short good_id = json_object_get_int(json_object_object_get(json_arr_tmp, "goods_no"));
		short stat = json_object_get_int(json_object_object_get(json_arr_tmp, "status"));
		int remain_ele = json_object_get_int(json_object_object_get(json_arr_tmp, "remain_electricity"));
		// printf("uav[%d]ele:%d\n", uav_no, remain_ele);
		//找到对应的无人机参数表中的索引
		int type_idx = atoi(json_object_to_json_string(json_object_object_get(json_arr_tmp, "type")) + 2);
		//开始更新uavs数组！！
		uavs_tmp[uav_no].curr_xyz[0] 		= cdnt_x;
		uavs_tmp[uav_no].curr_xyz[1] 		= cdnt_y;
		uavs_tmp[uav_no].curr_xyz[2] 		= cdnt_z;
		uavs_tmp[uav_no].good_id 			= good_id;
		uavs_tmp[uav_no].status 			= stat;
		uavs_tmp[uav_no].remain_electricity = remain_ele;
		if (stat != UAV_STAT_DESTROYED)
			game_paras_tmp->uavs_real_num++;
		else
		{
			game_paras_tmp->is_attack_uav[uav_no] = 0; 
			//如果是本来回家充电的，要更新统计数
			if (   uavs_tmp[uav_no].aim_xyz[0] == we_parking[0] //如果是要回去充电的uav，
	            && uavs_tmp[uav_no].aim_xyz[1] == we_parking[1]
	            && uavs_tmp[uav_no].aim_xyz[2] == 0 )
			{
				game_paras_tmp->back_charge_uavs_num--;
#ifdef PRINT_CHARGE_NUM				
				printf("cha->destroyed\n");
#endif
			}
	            
		}
		uavs_tmp[uav_no].uav_para = &(UAVs_para[type_idx]);
#ifdef PRINT_DISTRIBUTION
		// printf("uav%d_wei:%d\n", uav_no, uavs_tmp[uav_no].uav_para->load_weight);
#endif
		//如果之前已分配过目标，并且没挂
		if (uavs_tmp[uav_no].aim_good != NULL && uavs_tmp[uav_no].status != UAV_STAT_DESTROYED) 
		{
			// if (uavs_tmp[uav_no].route->next != NULL)//如果还没到达
			{
				//如果已不能被拾起了
				if (uavs_tmp[uav_no].aim_good->status != GOOD_STAT_NORMAL || uavs_tmp[uav_no].aim_good->left_time <= 0) 
				{
					if (!(uavs_tmp[uav_no].curr_xyz[0] == uavs_tmp[uav_no].aim_xyz[0] && uavs_tmp[uav_no].curr_xyz[1] == uavs_tmp[uav_no].aim_xyz[1]))
					{
						uavs_tmp[uav_no].aim_good = NULL;//清空其目标good
						memset(uavs_tmp[uav_no].aim_xyz, -1, sizeof(uavs_tmp[uav_no].aim_xyz));//清空目标good坐标
						FreeRoute(uavs_tmp[uav_no].route);//清空路径！
					}	
				}
			}
			// else
			// if (uavs_tmp[uav_no].good_id != -1)//如果刚到达，被自己拾起了
			// {
			// 	//更新目标坐标
			// 	uavs_tmp[uav_no].aim_xyz[0] = uavs_tmp[uav_no].aim_good->end_xyz[0];
			// 	uavs_tmp[uav_no].aim_xyz[1] = uavs_tmp[uav_no].aim_good->end_xyz[1];
			// 	uavs_tmp[uav_no].aim_xyz[2] = uavs_tmp[uav_no].aim_good->end_xyz[2];
			// 	uavs_tmp[uav_no].aim_good = NULL;//清空其目标good
			// 	// uavs_tmp[uav_no].
			// 	FreeRoute(uavs_tmp[uav_no].route);//清空路径！
			// }
		}
	}
	
	//attention!!  not json_object_put()!!!
	json_obj_uav_tmp = NULL; 
	return 0;
}

/*************************************************
@Description: 更新goods的信息
@Input: 货物信息json数组  所有货物结构体数组
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int UpdateGoodsInfo(struct _game_para* game_paras, json_object* json_obj_goods_uavs, struct _good* good)
{
	struct _good* good_tmp = good;
	json_object* json_obj_goods_uavs_tmp = json_obj_goods_uavs;
	json_object* json_obj_goods_tmp = json_object_object_get(json_obj_goods_uavs_tmp, "goods");
	json_object* json_obj_uavs_tmp = json_object_object_get(json_obj_goods_uavs_tmp, "UAV_we");

	int goods_num_tmp = json_object_array_length(json_obj_goods_tmp);
	int max_good_num_tmp = 0;
	if (goods_num_tmp != 0)
		max_good_num_tmp = json_object_get_int(json_object_object_get(json_object_array_get_idx(json_obj_goods_tmp, goods_num_tmp - 1), "no"));
	
	if (game_paras->goods_num < (1 + max_good_num_tmp))
		game_paras->goods_num = 1 + max_good_num_tmp;
	int uavs_num_tmp = json_object_array_length(json_obj_uavs_tmp);
	//为了清空列表中已不存在的good
	for (int i = 0; i < game_paras->goods_num; ++i)
	{
		memset(good[i].start_xyz, -1, sizeof(good[i].start_xyz));
        memset(good[i].end_xyz, -1, sizeof(good[i].end_xyz));
        good[i].weight = 0;
        good[i].value = 0;
        good[i].start_time = -1;
        good[i].remain_time = -1;
        good[i].status = GOOD_STAT_DISAPPEAR; //-1为不存在
        // memset(good[i].aimed_uavs, 0, sizeof(good[i].aimed_uavs));
        // good[i].aimed_uavs_num = 0;
	}
	for (int i = 0; i < goods_num_tmp; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_obj_goods_tmp, i);
		int good_no = json_object_get_int(json_object_object_get(json_arr_tmp, "no"));		
		//然后再清理下面的
		short st_x = json_object_get_int(json_object_object_get(json_arr_tmp, "start_x"));
		short st_y = json_object_get_int(json_object_object_get(json_arr_tmp, "start_y"));
		short ed_x = json_object_get_int(json_object_object_get(json_arr_tmp, "end_x"));
		short ed_y = json_object_get_int(json_object_object_get(json_arr_tmp, "end_y"));
		int weight = json_object_get_int(json_object_object_get(json_arr_tmp, "weight"));
		int value = json_object_get_int(json_object_object_get(json_arr_tmp, "value"));
		int st_tm = json_object_get_int(json_object_object_get(json_arr_tmp, "start_time"));
		int rm_tm = json_object_get_int(json_object_object_get(json_arr_tmp, "remain_time"));
		int lt_tm = json_object_get_int(json_object_object_get(json_arr_tmp, "left_time"));
		int stat = json_object_get_int(json_object_object_get(json_arr_tmp, "status"));
		//开始更新uavs数组！！
		good_tmp[good_no].start_xyz[0] = st_x;
		good_tmp[good_no].start_xyz[1] = st_y;
		good_tmp[good_no].start_xyz[2] = 0;
		good_tmp[good_no].end_xyz[0] = ed_x;
		good_tmp[good_no].end_xyz[1] = ed_y;
		good_tmp[good_no].end_xyz[2] = 0;
		good_tmp[good_no].weight = weight;
		good_tmp[good_no].value = value;
		good_tmp[good_no].start_time = st_tm;
		good_tmp[good_no].remain_time = rm_tm;
		good_tmp[good_no].left_time = lt_tm;
		good_tmp[good_no].status = stat;
		if (stat != GOOD_STAT_NORMAL) //如果该货物已经不能被继续拾起了，就不需要uav来锁定了
		{
			memset(good_tmp[good_no].aimed_uavs, 0, sizeof(good_tmp[good_no].aimed_uavs));
			good_tmp[good_no].aimed_uav_id	= -1;
			good_tmp[good_no].aimed_uavs_num = 0;
			FreeRoute(good_tmp[good_no].transport_route);
			good_tmp[good_no].transport_steps_num = 0;
		}
		else//如果还能被继续拾起，则更新目标uavs数组
		{
			for (int j = 0; j < uavs_num_tmp; ++j)
			{
				json_arr_tmp = json_object_array_get_idx(json_obj_uavs_tmp, j);
				int uav_no = json_object_get_int(json_object_object_get(json_arr_tmp, "no"));
				int uav_stat = json_object_get_int(json_object_object_get(json_arr_tmp, "status"));
				if (uav_stat == UAV_STAT_DESTROYED) //如果无人机已不存在了
				{
					good_tmp[good_no].aimed_uavs[uav_no] = 0;//则置空相应位
					good_tmp[good_no].aimed_uav_id	= -1;
				}
			}
			good_tmp[good_no].aimed_uavs_num = CountUAVsNum(&(good_tmp[good_no]));
		}
	}
	for (int i = 0; i < game_paras->goods_num; ++i)
		game_paras->goods_val_sort[i] = i;
	BubbleSortGoods(good_tmp, game_paras, game_paras->goods_num);
	// for (int i = 0; i < game_paras->goods_num; ++i)
 //        printf("id%d:%d ", i, game_paras->goods_val_sort[i]);
	json_obj_goods_uavs_tmp = NULL;
	return 0;
}

/*************************************************
@Description: 释放UAVs数组的内存
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int FreeUAVsInfo(struct _uav* u)
{
	for (int i = 0; i < UAV_MAX_NUM; ++i)
	{
		FreeRoute(u[i].route);
		free(u[i].route);//head point!!
	}
	free(UAVs_para);
	free(UAVs_para_id);

	return 0;
}
/*************************************************
@Description: 释放goods数组的内存
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int FreeGoodsInfo(struct _good* g)
{
	for (int i = 0; i < GOOD_MAX_NUM; ++i)
	{
		FreeRoute(g[i].transport_route);
		free(g[i].transport_route);//head point!!
	}
	return 0;	
}
/*************************************************
@Description: 计算每个good被几个uav锁定了
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int CountUAVsNum(struct _good* const g)
{
	int ret = 0;
	for (int i = 0; i < UAV_MAX_NUM; ++i)
		ret += g->aimed_uavs[i];
	return ret;
}

/*************************************************
@Description: 计算某货物和某uav的匹配度
@Input: 
@Output: 
@Return: 匹配度 
@Others: 范围：1-STBLT_CVT_RANGE（1600）
*************************************************/
unsigned int CalculateSuitability(struct _good* const g, struct _uav* const u)
{
	unsigned int stblt = 0;//printf("limi:%d\n", low_limit);
	if (g->weight > u->uav_para->load_weight)
	{
		stblt = 0;
	}else//开始为0的状态也可以分配，因为可以早点计算good的bfs，不然就浪费开始的时间了
	if (u->remain_electricity != 0 && u->remain_electricity < g->weight * (g->transport_steps_num + GOOD_TRANS_MARGIN))//如果电量不够搬运的话   +1是因为刚下去取到good时也会耗电
	{
		stblt = 0;
	}else//如果最快直线跑过去也赶不上的话
	if ((u->curr_xyz[0] == g->start_xyz[0] ? abs(u->curr_xyz[1] - g->start_xyz[1]) 
		: (u->curr_xyz[1] == g->start_xyz[1] ? abs(u->curr_xyz[0] - g->start_xyz[0]) 
			: (sqrt((pow(u->curr_xyz[0] - g->start_xyz[0], 2) + pow(u->curr_xyz[1] - g->start_xyz[1], 2)) / 2))))
		 + abs(u->curr_xyz[2] - low_limit) + abs(g->start_xyz[2] - low_limit) > g->left_time)
	{
		stblt = 0;
	}else
	{
		//最大80000
		unsigned int dis = pow(g->start_xyz[0] - u->curr_xyz[0], 2) + pow(g->start_xyz[1] - u->curr_xyz[1], 2);
		//最大给1000 000 000, 3次方是增大重量匹配度的影响，减小距离匹配度的影响
		unsigned int wht_diff = pow((u->uav_para->load_weight - g->weight) > 1000 ? 1000 : (u->uav_para->load_weight - g->weight), 3);
		stblt = (STBLT_CVT_RANGE - dis - wht_diff > 0 ? STBLT_CVT_RANGE - dis - wht_diff : 0);
	}
	return stblt;
}
/*************************************************
@Description: 按good的价值高低将goods_val_sort按从大到小排序，得到从大到小的goods_no
@Input: 
@Output: 
@Return: 
@Others: 倒序！从大到小！！
*************************************************/
int BubbleSortGoods(struct _good* goods, struct _game_para* game_para, int size)
{
	int low = 0, high = size - 1; 
	
    while (low < high) 
    {    
        for (int i = low; i < high; ++i) //正向冒泡,找到最小者 
        {
        	if (goods[game_para->goods_val_sort[i]].value < goods[game_para->goods_val_sort[i + 1]].value) 
        	{
				int tmp = game_para->goods_val_sort[i]; 
				game_para->goods_val_sort[i] = game_para->goods_val_sort[i + 1]; 
				game_para->goods_val_sort[i + 1] = tmp;
			} 
        }   	
        --high;//修改high值, 前移一位    
        for (int i = high; i > low; --i) //反向冒泡,找到最大者 
        {
        	if (goods[game_para->goods_val_sort[i]].value > goods[game_para->goods_val_sort[i - 1]].value) 
        	{    
                int tmp = game_para->goods_val_sort[i]; 
                game_para->goods_val_sort[i] = game_para->goods_val_sort[i - 1]; 
                game_para->goods_val_sort[i - 1] = tmp;
            } 
        }         
        ++low;//修改low值,后移一位    
    }
    return 0;
}
/*************************************************
@Description: 根据每个uav的value值将uavs_val_sort数组从小到大排序，得到从小到大的uavpara id
@Input: 
@Output: 
@Return: 
@Others: 从小到大
*************************************************/
int BubbleSortParas(struct _game_para* game_paras_tmp, struct _uav_para* para, int size)
{
	//start from 1 to size !!!!
	int low = 1, high = size; 
	
    while (low < high) 
    {    
        for (int i = low; i < high; ++i) //正向冒泡,找到最大者 
        {
        	if (para[game_paras_tmp->uavs_val_sort[i]].value > para[game_paras_tmp->uavs_val_sort[i + 1]].value) 
        	{
				short tmp = game_paras_tmp->uavs_val_sort[i];
				game_paras_tmp->uavs_val_sort[i] = game_paras_tmp->uavs_val_sort[i + 1];
				game_paras_tmp->uavs_val_sort[i + 1] = tmp;
			} 
        }   	
        --high;//修改high值, 前移一位    
        for (int i = high; i > low; --i) //反向冒泡,找到最小者 
        {
        	if (para[game_paras_tmp->uavs_val_sort[i]].value < para[game_paras_tmp->uavs_val_sort[i - 1]].value) 
        	{    
                short tmp = game_paras_tmp->uavs_val_sort[i];
				game_paras_tmp->uavs_val_sort[i] = game_paras_tmp->uavs_val_sort[i - 1];
				game_paras_tmp->uavs_val_sort[i - 1] = tmp;
            } 
        }         
        ++low;//修改low值,后移一位    
    }
    return 0;	
}

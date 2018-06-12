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

#include "map.h"
#include "config.h"
#include "route.h"


extern struct _uav_para* UAVs_para;//以type数作为索引
extern short* UAVs_para_id;
short we_parking[2] = {-2, -2};
unsigned short low_limit = 0;
/*************************************************
@Description: 地图的初始化
@Input: 服务器开始给的json的map键的值    地图指针
@Output: 地图指针
@Return: 0-成功   1-失败  
@Others: 	
			x  x  x  x | x  x  x  x
			|  |  |__|___|__|__|__|___表示无人机价值等级，暂定区间为 (0 - 61)+2, 0表示无物体，1表示障碍物
			|  |______________________表示是否是雾区    	0-不是雾区  	1-是雾区
			|_________________________表示阵营         	0-我方  		1-敌方
*************************************************/
int MapInit(json_object* json_obj_map, struct _map* map)
{
	json_object* json_obj_map_tmp = json_obj_map;
	struct _map* map_tmp = map;
	//战场长宽高
	map_tmp->battle_x = json_object_get_int(json_object_object_get(json_object_object_get(json_obj_map_tmp, "map"), "x"));
	map_tmp->battle_y = json_object_get_int(json_object_object_get(json_object_object_get(json_obj_map_tmp, "map"), "y"));
	map_tmp->battle_z = json_object_get_int(json_object_object_get(json_object_object_get(json_obj_map_tmp, "map"), "z"));
	// printf("battle:x:%d,y:%d,z:%d\n", map_tmp->battle_x, map_tmp->battle_y, map_tmp->battle_z);
	//注意此处的动态分配内存（堆）！！
	map_tmp->map = (char**)malloc(sizeof(char*) * map_tmp->battle_z);
	for (int i = 0; i < map_tmp->battle_z; ++i)
	{
		map_tmp->map[i] = (char*)malloc(sizeof(char) * map_tmp->battle_x * map_tmp->battle_y);
		//记得malloc之后一定要初始化！
		for (int j = 0; j < map_tmp->battle_x * map_tmp->battle_y; ++j)
			map_tmp->map[i][j] = 0;
	}
	//停机坪
	map_tmp->parking_x = json_object_get_int(json_object_object_get(json_object_object_get(json_obj_map_tmp, "parking"), "x"));
	map_tmp->parking_y = json_object_get_int(json_object_object_get(json_object_object_get(json_obj_map_tmp, "parking"), "y"));
	we_parking[0] = map_tmp->parking_x;
	we_parking[1] = map_tmp->parking_y;	printf("we_parking:%d, %d\n", we_parking[0], we_parking[1]);
	//飞行高度限制
	map_tmp->fly_low_limit  = json_object_get_int(json_object_object_get(json_obj_map_tmp, "h_low"));//printf("fly_low_limit%d\n", map_tmp->fly_low_limit);
	low_limit = map_tmp->fly_low_limit;
	map_tmp->fly_high_limit = json_object_get_int(json_object_object_get(json_obj_map_tmp, "h_high"));

	map_tmp->enemy_parking_x = 0;
	map_tmp->enemy_parking_y = 0;
	// printf("parking:x:%d,y:%d\n", map_tmp->parking_x, map_tmp->parking_y);
	// printf("limit:low:%d,high:%d\n", map_tmp->fly_low_limit, map_tmp->fly_high_limit);
	//构建大地图！！
	//开始的时候停机坪置为1
	SetMapValue(map_tmp, map_tmp->parking_x, map_tmp->parking_y, 0, 1);
	//building障碍物置为1
	json_object* json_val = json_object_object_get(json_obj_map_tmp, "building");
	int buildings_num = json_object_array_length(json_val);

	int floors_num = 1;
	char floors_check[200] = {0};//防止出现重复的层
	short floors_tmp[200] = {0};
	floors_tmp[0] = map_tmp->fly_low_limit;
	floors_check[map_tmp->fly_low_limit] = 1;

	for (int i = 0; i < buildings_num; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_val, i);
		int x = json_object_get_int(json_object_object_get(json_arr_tmp, "x"));
		int y = json_object_get_int(json_object_object_get(json_arr_tmp, "y"));
		int l = json_object_get_int(json_object_object_get(json_arr_tmp, "l"));
		int w = json_object_get_int(json_object_object_get(json_arr_tmp, "w"));
		int h = json_object_get_int(json_object_object_get(json_arr_tmp, "h"));
		//h已经表示了顶层上面的z坐标
		if ( !floors_check[h] && h > map_tmp->fly_low_limit && h <= map_tmp->fly_high_limit)
		{
			// printf("floors_tmp[%d]:%d\n", floors_num, h);
			floors_tmp[floors_num++] = h;
			floors_check[h] = 1;
		}
		//水平上坐标位置为x->x+l-1, y->y+w-1 0->h-1
		for (int h_cnt = 0; h_cnt < h; ++h_cnt)
			for (int w_cnt = 0; w_cnt < w; ++w_cnt)
				for (int l_cnt = 0; l_cnt < l; ++l_cnt)
					SetMapValue(map_tmp, x + l_cnt, y + w_cnt, h_cnt, 0x01);
	}
	//雾区
	json_val = json_object_object_get(json_obj_map_tmp, "fog");
	int fogs_num = json_object_array_length(json_val);
	for (int i = 0; i < fogs_num; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_val, i);
		int x = json_object_get_int(json_object_object_get(json_arr_tmp, "x"));
		int y = json_object_get_int(json_object_object_get(json_arr_tmp, "y"));
		int l = json_object_get_int(json_object_object_get(json_arr_tmp, "l"));
		int w = json_object_get_int(json_object_object_get(json_arr_tmp, "w"));
		int b = json_object_get_int(json_object_object_get(json_arr_tmp, "b"));
		int t = json_object_get_int(json_object_object_get(json_arr_tmp, "t"));
		//t表示雾区的最大高度，水平上坐标为x->x+l-1, y->y+w-1，垂直区间为b->t"
		for (int h_cnt = b; h_cnt < t + 1; ++h_cnt)
			for (int w_cnt = 0; w_cnt < w; ++w_cnt)
				for (int l_cnt = 0; l_cnt < l; ++l_cnt)
				{//因为雾区和障碍物可能重叠，不能影响低位
					int val_tmp = GetPointVal(map_tmp, x + l_cnt, y + w_cnt, h_cnt);
					SetMapValue(map_tmp, x + l_cnt, y + w_cnt, h_cnt, val_tmp | 0x40);
				}
	}
	//更新map结构体里的graph成员！图的初始化！！
	//从低到上排序、！
	BubbleSortFloors(floors_tmp, floors_num);
	//限制graph最大数
	floors_num = (floors_num > GRAPH_MAX_NUM ? GRAPH_MAX_NUM : floors_num);
	map_tmp->graphs_num = floors_num;
	//之前耗时太长是因为memset的使用，现改为GraphInit里的循环初始化!!
	map_tmp->graphs = (struct _graph*)malloc(sizeof(struct _graph) * floors_num);
	for (int i = 0; i < floors_num; ++i)
	{
		map_tmp->graphs[i].floor_z = floors_tmp[i];
		// printf("[%d]%d\n", i, floors_tmp[i]);
	}
	// printf("floors_num:%d\n", map_tmp->graphs[5].floor_z);
	// //高的层初始化为最低层，防止bug!
	// for (int i = floors_num; i < GRAPH_MAX_NUM; ++i)
	// 	map_tmp->graphs[i].floor_z = map_tmp->fly_low_limit;
	//此处图的初始化包括点的初始化，和邻接矩阵的初始化
	GraphInit(map_tmp, map_tmp->graphs);
	for (int i = 0; i < map_tmp->graphs_num; ++i)
		CreateGraph(map_tmp, &(map_tmp->graphs[i]));
	// for (int i = 0; i < map_tmp->graphs[0].vertex_num; ++i)
	// {
	// 	for (int j = 0; j < map_tmp->graphs[0].vertex_num; ++j)
	// 	{
	// 		printf("[%d][%d]:%d ", i, j, map_tmp->graphs[0].adj[i][j]);
	// 	}
	// }
	// PrintGraphAdj(map_tmp, &(map_tmp->graphs[0]), 2, 17);

	//待命区的确定！
	map_tmp->standby = (int*)malloc(sizeof(int) * map_tmp->battle_x * map_tmp->battle_y);
	int* dis_tmp = (int*)malloc(sizeof(int) * map_tmp->battle_x * map_tmp->battle_y);
	// 实际上是地图中心偏左下角，因为没有浮点运算
	int middle_xy = (map_tmp->battle_y - 1) / 2 * map_tmp->battle_x + (map_tmp->battle_x - 1) / 2;
	//确定半径,只会偏小，因为也没有浮点运算
	int r = pow((map_tmp->battle_x > map_tmp->battle_y ? map_tmp->battle_y : map_tmp->battle_x) / STANDBY_RADIUS_SCALE, 2);
	//dis_tmp是为了冒泡排序用
	for (int i = 0; i < map_tmp->battle_x * map_tmp->battle_y; ++i)
	{
		map_tmp->standby[i] = i;
		dis_tmp[i] = abs(CalculateDistancePow(map_tmp, i, middle_xy) - r);
	}
	BubbleSort(dis_tmp, map_tmp->standby, map_tmp->battle_x * map_tmp->battle_y);
	free(dis_tmp);

	map_tmp->home_state = HOME_STAT_NONE;

	json_obj_map_tmp = NULL;
	return 0;
}

/*************************************************
@Description: 根据坐标和值更新地图某个点的值
@Input: 大地图指针  坐标(x/y/z)  需要置的值
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int SetMapValue(struct _map* map, short x, short y, short z, unsigned char val)
{
	struct _map* map_tmp = map;
	int arr_idx = y * map_tmp->battle_x + x;
	map_tmp->map[z][arr_idx] = val & 0xff;		

	return 0;
}

/*************************************************
@Description: 得到路径下一个点的地图值
@Input: 
@Output: 
@Return: char
@Others: 此处map已经更新过了
*************************************************/
char GetPointVal(const struct _map* map, const short x, const short y, const short z)
{
	int xy_tmp = y * map->battle_x + x;
	unsigned char val = map->map[z][xy_tmp];

	return val;
}

/*************************************************
@Description: 检测地图上某个点是不是某个值
@Input: 大地图指针  xy   z    值
@Output: 
@Return: 0-相等   1-不相等
@Others: 
*************************************************/
// int CheckMapValue(const struct _map* map, unsigned int xy, unsigned short z, char val)
// {
// 	return (map->map[z][xy] == val ? 0 : 1);
// }
/*************************************************
@Description: 根据服务器返回的json值更新大地图信息
@Input: 大地图指针  敌方或我方uav数据  敌方还是我方
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
static short park_flag = 0;
int UpdateMap(json_object* json_obj_uav, struct _map* map, struct _uav* uavs, struct _good* goods, int camp)
{
	struct _map* map_tmp = map;
	struct _uav* uavs_tmp = uavs;
	struct _good* goods_tmp = goods;
	json_object* json_obj_uav_tmp = json_obj_uav;
	//要先清空整个大地图！记住，除了一开始初始化的障碍物，停机坪和雾区不能动,而且更新对方uav时就不用重新初始化了！!!
	if (camp == 1)
	{
		for (int i = 0; i < map_tmp->battle_z; ++i)
			for (int j = 0; j < map_tmp->battle_x * map_tmp->battle_y; ++j)
				if ((map_tmp->map[i][j]& 0x3f) != 0x01)	//障碍物 1 不能动
					map_tmp->map[i][j] &= 0x40; //雾区不能动
	}	
	//重新标
	int uavs_num_tmp = json_object_array_length(json_obj_uav_tmp);
	for (int i = 0; i < uavs_num_tmp; ++i)
	{
		json_object* json_arr_tmp = json_object_array_get_idx(json_obj_uav_tmp, i);
		short stat = json_object_get_int(json_object_object_get(json_arr_tmp, "status"));
		if (stat == 1) //如果已坠毁的，不考虑
			continue;
		int uav_no = json_object_get_int(json_object_object_get(json_arr_tmp, "no"));
		short cdnt_x = json_object_get_int(json_object_object_get(json_arr_tmp, "x"));
		short cdnt_y = json_object_get_int(json_object_object_get(json_arr_tmp, "y"));
		short cdnt_z = json_object_get_int(json_object_object_get(json_arr_tmp, "z"));
		short good_no = json_object_get_int(json_object_object_get(json_arr_tmp, "goods_no"));
		//uav的价值权重	
		// printf("setting %d...\n", uav_no);
		if (camp == 1) //如果是我方的
		{
			unsigned char level = (uavs_tmp[uav_no].uav_para->value + (good_no != -1 ? goods_tmp[good_no].value : 0)) 
							>= (61 * UAV_VAL_CVT_BASE) 
							? 0x3d : ((uavs_tmp[uav_no].uav_para->value + (good_no != -1 ? goods_tmp[good_no].value : 0)) / UAV_VAL_CVT_BASE);
			if (stat == 0) //如果不在雾区
				SetMapValue(map_tmp, cdnt_x, cdnt_y, cdnt_z, level + 2);
			else
			if (stat == 2) //如果在雾区
				SetMapValue(map_tmp, cdnt_x, cdnt_y, cdnt_z, (level + 2) | 0x40);
		}else
		if (camp == -1)//如果是敌方的
		{
			if (park_flag == 0)
			{
				map_tmp->enemy_parking_x = cdnt_x;
				map_tmp->enemy_parking_y = cdnt_y;
				printf("enemy_parking:x, %d, y, %d\n",map_tmp->enemy_parking_x, map_tmp->enemy_parking_y );
				park_flag = 1;
			}		

			int idx = atoi(json_object_to_json_string(json_object_object_get(json_arr_tmp, "type")) + 2);
			int k = 0;
			while(k < UAV_TYPE_MAX_NUM && UAVs_para_id[k++] != idx);
			k = (k < UAV_TYPE_MAX_NUM ? k : 0);
			
			unsigned char level = (UAVs_para[k].value + (good_no != -1 ? goods_tmp[good_no].value : 0))
					>= (61 * UAV_VAL_CVT_BASE) 
					? 0x3d : ((UAVs_para[k].value + (good_no != -1 ? goods_tmp[good_no].value : 0)) / UAV_VAL_CVT_BASE);
			if (stat != UAV_STAT_IN_FOG) //如果不在雾区
				SetMapValue(map_tmp, cdnt_x, cdnt_y, cdnt_z, 0x80 | (level + 2));
		}
	}
	//只要老家没有自己uav就置空老家状态！
	short cnt_tmp = 0;
	for (int i = 1; i <= map_tmp->fly_low_limit; ++i)
	{
		char val_tmp = GetPointVal(map_tmp, map_tmp->parking_x, map_tmp->parking_y, i);
		if (!((val_tmp & 0x80) == 0x00 && (val_tmp & 0x3f) >= 0x02))//如果没有自己uav
			cnt_tmp++;
	}
	if (cnt_tmp == map_tmp->fly_low_limit)//如果老家上方所有点都没己方uav，就置空，否则就保持原样
	{
		map_tmp->home_state = HOME_STAT_NONE;
		// game_para->back_charge_uavs_num = 0;
	}


	json_obj_uav_tmp = NULL;
	return 0;
}

/*************************************************
@Description: 释放地图的内存
@Input: 地图指针
@Output: 地图指针
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int FreeMap(struct _map* map)
{
	for (int i = 0; i < map->battle_z; ++i)
	   	free(map->map[i]); 
    free(map->map); 
#ifdef GOOD_BFS_IMPROVED 
    for (int i = 0; i < map->battle_x * map->battle_y; ++i)
	   	free(map->all_path[i]); 
    free(map->all_path); 
#endif
    free(map->graphs);
    free(map->standby);

    return 0;
}

/*************************************************
@Description: 释放地图的内存
@Input: 地图指针
@Output: 地图指针
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int PrintMap(struct _map* map)
{
	struct _map* map_tmp = map;
	for (int i = 0; i < map_tmp->battle_z; ++i)
	{
		for (int j = 0; j < map_tmp->battle_x * map_tmp->battle_y; ++j)
			printf("0x%02x ", map_tmp->map[i][j]);
		printf("\n");
	}

	return 0;
}

/*************************************************
@Description: 计算一个平面内两个点之间的距离的平方
@Input: 
@Output: 
@Return: 距离  
@Others: 
*************************************************/
int CalculateDistancePow(struct _map* map, int cxy, int dxy)
{
	int cx = cxy % map->battle_x;
	int cy = cxy / map->battle_x;
	int dx = dxy % map->battle_x;
	int dy = dxy / map->battle_x;

	return (pow(cx - dx, 2) + pow(cy - dy, 2));
}

/*************************************************
@Description: 计算一个平面内两个点之间的距离的平方
@Input: 
@Output: 
@Return: 距离  
@Others: 
*************************************************/
int CalculateDistancePow2(short cx, short cy, short dx, short dy)
{
	return (pow(cx - dx, 2) + pow(cy - dy, 2));
}

/*************************************************
@Description: 冒泡排序，
@Input: src
@Output: ret
@Return:得到离某个点最近的距离排序数组
@Others: 
*************************************************/
int BubbleSort(int* src, int* ret, int size)
{
	int low = 0, high = size - 1; 
    while (low < high) 
    {    
        for (int i = low; i < high; ++i) //正向冒泡,找到最大者 
        {
        	if (src[i] > src[i + 1]) 
        	{
				int tmp = src[i]; src[i] = src[i + 1]; src[i + 1] = tmp;
				int id_tmp = ret[i]; ret[i] = ret[i + 1]; ret[i + 1] = id_tmp;
			} 
        }   	
        --high;//修改high值, 前移一位    
        for (int i = high; i > low; --i) //反向冒泡,找到最小者 
        {
        	if (src[i] < src[i - 1]) 
        	{    
                int tmp = src[i]; src[i] = src[i - 1]; src[i - 1] = tmp;
				int id_tmp = ret[i]; ret[i] = ret[i - 1]; ret[i - 1] = id_tmp;   
            } 
        }         
        ++low;//修改low值,后移一位    
    }
    return 0;
}
/*************************************************
@Description: 冒泡排序
@Input: 
@Output: 
@Return:
@Others: 从矮到高。。
*************************************************/
int BubbleSortFloors(short* floors, int size)
{
	int low = 0, high = size - 1; 
    while (low < high) 
    {    
        for (int i = low; i < high; ++i) //正向冒泡,找到最大者 
        {
        	if (floors[i] > floors[i + 1]) 
        	{
				short tmp = floors[i]; floors[i] = floors[i + 1]; floors[i + 1] = tmp;
			} 
        }   	
        --high;//修改high值, 前移一位    
        for (int i = high; i > low; --i) //反向冒泡,找到最小者 
        {
        	if (floors[i] < floors[i - 1]) 
        	{    
                short tmp = floors[i]; floors[i] = floors[i - 1]; floors[i - 1] = tmp;
            } 
        }         
        ++low;//修改low值,后移一位    
    }
    return 0;
}
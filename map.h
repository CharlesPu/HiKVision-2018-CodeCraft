/*******************************
@@Author     : Charles
@@Date       : 2018-05-10
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#ifndef __MYMAP_H__
#define __MYMAP_H__ 

#include "json.h"
#include "uavs.h"

#define HOME_STAT_NONE 0
#define HOME_STAT_OUT  1
#define HOME_STAT_IN   2

struct _map
{
	unsigned char** map; //真正的地图！
	unsigned short battle_x, battle_y, battle_z;
	unsigned short parking_x, parking_y;
	unsigned short enemy_parking_x, enemy_parking_y;
	unsigned short fly_low_limit, fly_high_limit;
	struct _graph* graphs;
	int graphs_num;//floors_num
	int* standby;//待命区
	short home_state;
#ifdef GOOD_BFS_IMPROVED 
	unsigned short** all_path;
#endif
};

int MapInit(json_object* json_obj, struct _map* map);
int SetMapValue(struct _map* map, short x, short y, short z, unsigned char val);
char GetPointVal(const struct _map* map, const short x, const short y, const short z);
// int CheckMapValue(const struct _map* map, unsigned int xy, unsigned short z, char val);
int UpdateMap(json_object* json_obj_uav, struct _map* map, struct _uav* uavs, struct _good* goods, int camp);
int FreeMap(struct _map* map);
int PrintMap(struct _map* map);

int CalculateDistancePow(struct _map* map, int cxy, int dxy);
int CalculateDistancePow2(short cx, short cy, short dx, short dy);
int BubbleSort(int*, int*, int size);
int BubbleSortFloors(short* floors, int size);

#endif
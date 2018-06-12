/*******************************
@@Author     : Charles
@@Date       : 2018-05-16
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#ifndef __ROUTE_H__
#define __ROUTE_H__ 

#include "map.h"
//图
struct _graph {
	int vertex_num;
	// int edge_num;
	int floor_z;
	char adj[GRAPH_VEX_MAX_NUM][GRAPH_VEX_MAX_NUM];//邻接矩阵
};
//队列
struct _queue {
	int front;
	int rear;
	int capacity;
	int* data;	
};

int FreeRoute(ROUTE_LIST);
void PrintRoute(ROUTE_LIST);
int PlanRoute(struct _map* map, struct _graph* graghs);
int GraphInit(struct _map* map, struct _graph* graghs);
int CreateGraph(const struct _map* map, struct _graph* gragh);
int BFS(struct _game_para* game_paras, struct _map* map, struct _graph* graghs, struct _uav* uav);
int GetPath(struct _map* map, struct _uav* uav, int* p, int f_z, int des);
int Good_BFS(struct _game_para* game_paras, struct _map* map, struct _graph* graghs, struct _good* g);
int Good_GetPath(struct _map* map, struct _good* g, int* p, int f_z, int des, int src);
int DeleteTopNode(ROUTE_LIST);
int InsertTopNode(ROUTE_LIST, const short x, const short y, const short z);
int HasBuilding(const struct _map* map, const short cx, const short cy, const short dx, const short dy, const short z);

int InsertQueue(struct _queue* Q, int x);
int DeleteQueue(struct _queue* Q);
void PrintQueue(struct _queue* Q);
void PrintGraphAdj(const struct _map* map, struct _graph* gragh, short x, short y);

#endif
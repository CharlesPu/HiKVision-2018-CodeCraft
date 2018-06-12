/*******************************
@@Author     : Charles
@@Date       : 2018-05-16
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "config.h"
#include "route.h"
#include "map.h"

/*************************************************
@Description: 清除某个UAV的路径
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int FreeRoute(ROUTE_LIST l)
{
	CDNT_NODE p, tmp;
	//头结点不能删！！
	p = l->next;
	l->next = NULL;
	while( p != NULL) 
	{
	    tmp = p->next;
	    free(p);
	    p = tmp;
	}

	return 0;
}
/*************************************************
@Description: 打印路径
@Input:  
@Output: 
@Return: 
@Others: 
*************************************************/
void PrintRoute(ROUTE_LIST l)
{
	CDNT_NODE tmp;
	tmp = l;
	while(tmp->next != NULL)
	{
		printf("(%d,%d,%d)->", tmp->next->cdnt_xyz[0], tmp->next->cdnt_xyz[1], tmp->next->cdnt_xyz[2]);
		tmp = tmp->next;
	}
	printf("\n");
}
/*************************************************
@Description: 规划每一层的路径！！
@Input:  map  graphs
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int PlanRoute(struct _map* map, struct _graph* graphs)
{
	/*struct _map* map_tmp = map;
	struct _graph* graphs_tmp = graphs;
	for (int i = 0; i < map_tmp->graphs_num; ++i)
	{
		CreateGraph(map_tmp, &(graphs_tmp[i]));
		BFS(graphs_tmp[i].vertex_num, graphs_tmp[i].dis, graphs_tmp[i].path);
	}*/
	return 0;
}

/*************************************************
@Description: 初始化图结构体的信息
@Input:  map   graphs  graphnum
@Output: 
@Return: 0-成功   1-失败  
@Others: 堆上初始化分配移到了TaskInit里了，以节约时间!!
*************************************************/
int GraphInit(struct _map* map, struct _graph* graphs)
{	
	int vertexs = map->battle_x * map->battle_y;
	vertexs = (vertexs > GRAPH_VEX_MAX_NUM ? GRAPH_VEX_MAX_NUM : vertexs);
#ifdef GOOD_BFS_IMPROVED 
	map->all_path = (unsigned short**)malloc(sizeof(unsigned short*) * vertexs);
    for (int i = 0; i < vertexs; ++i)
        map->all_path[i] = (unsigned short*)malloc(sizeof(unsigned short) * vertexs);
    for (int i = 0; i < vertexs; ++i)
        for (int j = 0; j < vertexs; ++j)
            map->all_path[i][j] = j;
#endif
	//edge = 6*(x-1)*(y-1)-[(y-1)*(x-2)+(y-2)*(x-1)]
	// int edge = 4 * map->battle_x * map->battle_y - 3 * map->battle_x - 3 * map->battle_y + 2;
	for (int k = 0; k < map->graphs_num; ++k)
	{
		graphs[k].vertex_num = vertexs;
		for (int i = 0; i < vertexs; ++i)
		{
			for (int j = 0; j < vertexs; ++j)
					graphs[k].adj[i][j] = 0;//adj矩阵初始化为0
		}
	}
	return 0;
}

/*************************************************
@Description: 创建图,主要是D矩阵
@Input:  map   graphs  graphnum
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int CreateGraph(const struct _map* map, struct _graph* graph)
{
	for (int v = 0; v < graph->vertex_num; ++v)//找到map每一层每个点相邻的上下左右4个点
	{
		if ((GetPointVal(map, v % map->battle_x, v / map->battle_x, graph->floor_z) & 0x3f) != 0x01)//首先要自己不是障碍区！！！！
		{
			//左
			if (v % map->battle_x != 0) //如果左边有点
			{
				if ((GetPointVal(map, (v - 1) % map->battle_x, (v - 1) / map->battle_x, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
				{
					graph->adj[v][v - 1] = 1;
					graph->adj[v - 1][v] = 1;
				}
				//左上
				if (v + map->battle_x < graph->vertex_num) //如果上边有点
					if ((GetPointVal(map, (v - 1) % map->battle_x, (v - 1) / map->battle_x + 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
					{
						graph->adj[v][v + map->battle_x - 1] = 1;
						graph->adj[v + map->battle_x - 1][v] = 1;
					}
				//左下
				if (v - map->battle_x >= 0) //如果下边有点
					if ((GetPointVal(map, (v - 1) % map->battle_x, (v - 1) / map->battle_x - 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
					{
						graph->adj[v][v - map->battle_x - 1] = 1;
						graph->adj[v - map->battle_x - 1][v] = 1;
					}
			}
			//右
			if (v % map->battle_x != map->battle_x - 1) //如果右边有点
			{
				if ((GetPointVal(map, (v + 1) % map->battle_x, (v + 1) / map->battle_x, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
				{
					graph->adj[v][v + 1] = 1;
					graph->adj[v + 1][v] = 1;
				}
				//右上
				if (v + map->battle_x < graph->vertex_num) //如果上边有点
					if ((GetPointVal(map, (v + 1) % map->battle_x, (v + 1) / map->battle_x + 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
					{
						graph->adj[v][v + map->battle_x + 1] = 1;
						graph->adj[v + map->battle_x + 1][v] = 1;
					}
				//右下
				if (v - map->battle_x >= 0) //如果xia边有点
					if ((GetPointVal(map, (v + 1) % map->battle_x, (v + 1) / map->battle_x - 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
					{
						graph->adj[v][v - map->battle_x + 1] = 1;
						graph->adj[v - map->battle_x + 1][v] = 1;
					}
			}
					
			//上
			if (v + map->battle_x < graph->vertex_num) //如果上边有点
				if ((GetPointVal(map, v % map->battle_x, v / map->battle_x + 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
				{
					graph->adj[v][v + map->battle_x] = 1;
					graph->adj[v + map->battle_x][v] = 1;
				}	
			//下
			if (v - map->battle_x >= 0) //如果下边有点
				if ((GetPointVal(map, v % map->battle_x, v / map->battle_x - 1, graph->floor_z) & 0x3f) != 0x01) //并且不是障碍区
				{
					graph->adj[v][v - map->battle_x] = 1;
					graph->adj[v - map->battle_x][v] = 1;
				}
			}
	}

	return 0;
}

/*************************************************
@Description: BFS算法！
@Input:  图   uav结构体
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int BFS(struct _game_para* game_paras, struct _map* map, struct _graph* graphs, struct _uav* uav)
{
	struct _graph* graphs_tmp = graphs;

	int src = uav->curr_xyz[1] * map->battle_x + uav->curr_xyz[0];
	int des = uav->aim_xyz[1] * map->battle_x + uav->aim_xyz[0];
	/*************修改des！！！分步BFS！！！***********/
	int distance_pow = CalculateDistancePow(map, src, des);//printf("pow:%d\n", distance_pow);
	float distance_pow_sqrt = sqrt(distance_pow);
	int step_length_tmp = UAV_STEP_MAX_LENGTH;
	int step_length_tmp_sqrt = UAV_STEP_MAX_LENGTH_SQRT;
	if (game_paras->battle_time == 0)
	{
		step_length_tmp = 140;
		step_length_tmp_sqrt = 11;
	}
	if (distance_pow > step_length_tmp)
	{
		float scale = step_length_tmp_sqrt / distance_pow_sqrt;//printf("scale:%f\n", scale);
		short change_x = uav->curr_xyz[0] + (int)((uav->aim_xyz[0] - uav->curr_xyz[0]) * scale);
		short change_y = uav->curr_xyz[1] + (int)((uav->aim_xyz[1] - uav->curr_xyz[1]) * scale);//printf("cx:%d,cy:%d\n", change_x, change_y);
		while((GetPointVal(map, change_x, change_y, map->fly_low_limit) & 0x3f) == 0x01)
		{
			change_x += (rand() % 3 - 1);//  -1  ~  1
			change_y += (rand() % 3 - 1);//  -1  ~  1
		}
		des = change_y * map->battle_x + change_x;//printf("deschage:%d```````````````````````\n", des);
	}
	//最终的路径
	int path[graphs_tmp[0].vertex_num];
	for (int i = 0; i < graphs_tmp[0].vertex_num; ++i)
		path[i] = -1;
	unsigned short steps_num = USHRT_MAX;//初始化为一个最大的数
	short floor = -1;

	for (int i = 0; i < map->graphs_num; ++i)// 
	{
#ifdef PRINT_BFS
		printf("uav_floor:%d\n", graphs_tmp[i].floor_z);
#endif
		if ((GetPointVal(map, uav->curr_xyz[0], uav->curr_xyz[1], graphs_tmp[i].floor_z) & 0x3f) == 0x01)//如果当前点zai这层为障碍物不要这层了
			continue;

		int vertex_n = graphs_tmp[i].vertex_num;
		char* visited = (char*)malloc(sizeof(char) * vertex_n);
		memset(visited, 0, sizeof(char) * vertex_n);
		int tmp = 0, flag = 0, steps_num_tmp = 0;
		//此次bfs的队列,并初始化！
		struct _queue q;
		q.data = (int*)malloc(sizeof(int) * vertex_n);
		for (int j = 0; j < vertex_n; ++j)
			q.data[j] = 0;
		q.capacity = vertex_n;
		q.front = 0; q.rear = 0;
		//此次bfs路径的暂存数组,初始化为-1！
		int parent[vertex_n];
		for (int j = 0; j < vertex_n; ++j)
			parent[j] = -1;
		visited[src] = 1;
		InsertQueue(&q, src);
	    while(q.front != q.rear && flag == 0)
	    {
	        tmp = DeleteQueue(&q);
	        // printf("des:%d,正在遍历顶点%d", des, tmp);
	        for(int j = 0; j < vertex_n; j++)
	        {
	            if( graphs_tmp[i].adj[j][tmp] != 0 && graphs_tmp[i].adj[tmp][j] != 0
	            	&& visited[j] == 0)
	            {
	            	parent[j] = tmp;
	                visited[j] = 1;
	                if(j == des) 
	                {
	            		flag = 1;break;
					}
					InsertQueue(&q, j);
	            }
	        }
			// printf(" flag=%d ", flag);
	        // PrintQueue(&q);
	    }
	    // for (int j = 0; j < vertex_n; ++j)
	    	// printf("[%d]%d\n", j, parent[j]);
	    //每一层找到目标点之后
	    int j = des;
	    while(parent[j] != -1)
	    {
	    	// printf("%d <- ", j);
	    	steps_num_tmp++;
	    	j = parent[j];
		}
		// printf("%d\n", j); 
		//找到真正的最小的路径，保存到path中！！
		steps_num_tmp += abs(graphs_tmp[i].floor_z - uav->curr_xyz[2]);//uav与此层差
		steps_num_tmp += abs(graphs_tmp[i].floor_z - uav->aim_xyz[2]);//目的地与此层差
		// printf("flor%d:%d\n", graphs_tmp[i].floor_z, steps_num_tmp);
		if (steps_num_tmp < steps_num )
		{
			steps_num = steps_num_tmp;
			floor = graphs_tmp[i].floor_z;
			for (int i = 0; i < graphs_tmp[0].vertex_num; ++i)
				path[i] = -1;
			memcpy(path, parent, sizeof(parent));
		}	
		//注意释放data的内存
		free(q.data);
		free(visited);
		if (HasBuilding(map, uav->curr_xyz[0], uav->curr_xyz[1], des % map->battle_x, des / map->battle_x//如果两点之间都没障碍物，就不用算其他层啦
		, (uav->aim_xyz[2] < uav->curr_xyz[2] ? uav->aim_xyz[2] : uav->curr_xyz[2])) )//后期再考虑两点之间的空间全是同一个building的情况！！！！
			break;
	}	
 	GetPath(map, uav, path, floor, des);

	return 0;
}

/*************************************************
@Description: 根据path矩阵得到总路径
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 去头留尾！
*************************************************/
int GetPath(struct _map* map, struct _uav* uav, int* pr, int f_z, int des)
{
	struct _map* map_tmp = map;
	struct _uav* uav_tmp = uav;

	CDNT_NODE head_tmp = (ROUTE_LIST)malloc(sizeof(struct _node));
	//头节点是0！！！多余的
	for (int i = 0; i < 3; ++i)
		head_tmp->cdnt_xyz[i] = 0;
	// memset(head_tmp->cdnt_xyz, 0, sizeof(head_tmp->cdnt_xyz));
	head_tmp->next = NULL;
	CDNT_NODE node_tmp = head_tmp; //活动节点指针！！始终指向链表中最后一个节点

	//到此平面的路径！！！
    if (uav_tmp->curr_xyz[2] > f_z) //如果在平面上方
    {
    	for (int j = uav_tmp->curr_xyz[2] - 1; j >= f_z; j--)
    	{
    		CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	    	tmp->cdnt_xyz[0] = uav_tmp->curr_xyz[0];
		    tmp->cdnt_xyz[1] = uav_tmp->curr_xyz[1];
		    tmp->cdnt_xyz[2] = j;  //z
		    tmp->next = NULL;
		    //更新活动节点指向！
		    node_tmp->next = tmp;
		    node_tmp = tmp;
    	}
    }else
    if (uav_tmp->curr_xyz[2] < f_z) //如果在平面下方
    {
    	for (int j = uav_tmp->curr_xyz[2] + 1; j <= f_z; j++)
    	{
    		CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	    	tmp->cdnt_xyz[0] = uav_tmp->curr_xyz[0];
		    tmp->cdnt_xyz[1] = uav_tmp->curr_xyz[1];
		    tmp->cdnt_xyz[2] = j;  //z
		    tmp->next = NULL;
		    //更新活动节点指向！
		    node_tmp->next = tmp;
		    node_tmp = tmp;
    	}
    }else//如果就在此平面了
    {
    }

	//在此平面的路径！
	// int des = uav_tmp->aim_xyz[1] * map_tmp->battle_x + uav_tmp->aim_xyz[0];
	// int src = uav_tmp->curr_xyz[1] * map_tmp->battle_x + uav_tmp->curr_xyz[0];
	int i = des;
	while(pr[i] != -1) 
	{
	    //分配新节点,注意是倒着插入！！
	    CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	    tmp->cdnt_xyz[0] = i % map_tmp->battle_x;  //x
	    tmp->cdnt_xyz[1] = i / map_tmp->battle_x;  //y
	    tmp->cdnt_xyz[2] = f_z;  //z
	    //node_tmp不变！始终指向要插入的位置的前面一个
	    tmp->next = node_tmp->next;
	    node_tmp->next = tmp;

	    i = pr[i];
	}
	//将node_tmp指向链表末尾！
	while(node_tmp->next != NULL)
		node_tmp = node_tmp->next;
    //下降路径
    if (des == uav_tmp->aim_xyz[1] * map_tmp->battle_x + uav_tmp->aim_xyz[0])//如果没到则不能下降
    {
    	for (int j = f_z - 1; j >= uav_tmp->aim_xyz[2]; j--)
	    {
	    	CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	    	tmp->cdnt_xyz[0] = uav_tmp->aim_xyz[0];
		    tmp->cdnt_xyz[1] = uav_tmp->aim_xyz[1];
		    tmp->cdnt_xyz[2] = j;  //z
		    tmp->next = NULL;
		    //更新活动节点指向！
		    node_tmp->next = tmp;
		    node_tmp = tmp;
	    }
    }
    
	//注意有头结点
	uav_tmp->route->next = head_tmp->next;
	//头结点释放掉！
	free(head_tmp);

	return 0;
}
#ifdef GOOD_BFS_IMPROVED 
int find_full_route_cnt = 0, fine_some_route_cnt = 0;
#endif
/*************************************************
@Description: 搬运货物的路径BFS！！
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int Good_BFS(struct _game_para* game_paras, struct _map* map, struct _graph* graphs, struct _good* good)
{
	struct _graph* graphs_tmp = graphs;

	int src = good->start_xyz[1] * map->battle_x + good->start_xyz[0];
	int des = good->end_xyz[1] * map->battle_x + good->end_xyz[0];
	//最终的路径
	int path[graphs_tmp[0].vertex_num];
	for (int i = 0; i < graphs_tmp[0].vertex_num; ++i)
		path[i] = -1;
	unsigned short steps_num = USHRT_MAX;//初始化为一个最大的数
	short floor = -1;

	for (int i = 0; i < GRAPH_GOOD_MAX_NUM; ++i)// 
	{
#ifdef PRINT_BFS
		printf("good_floor:%d\n", graphs_tmp[i].floor_z);
#endif
		int steps_num_tmp = 0;
		int vertex_n = graphs_tmp[i].vertex_num;
#ifdef GOOD_BFS_IMPROVED 
		if (map->all_path[src][des] != des)//说明在全局路径中找到了
		{
			printf("find full !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! %d !\n", find_full_route_cnt++);
			//此次bfs路径的暂存数组,初始化为-1！
			int parent[vertex_n];
			for (int j = 0; j < vertex_n; ++j)
				parent[j] = -1;
			int src_tmp = src;
			while (src_tmp != des)
			{
				int tmp = map->all_path[src_tmp][des];
				parent[tmp] = src_tmp;
				src_tmp = tmp;
				steps_num_tmp++;
			}
			// printf("%d\n", j); 
			//找到真正的最小的路径，保存到path中！！
			steps_num_tmp += abs(graphs_tmp[i].floor_z - good->start_xyz[2]);//uav与此层差
			steps_num_tmp += abs(graphs_tmp[i].floor_z - good->end_xyz[2]);//目的地与此层差
			// printf("flor%d:%d\n", graphs_tmp[i].floor_z, steps_num_tmp);
			if (steps_num_tmp < steps_num )
			{
				steps_num = steps_num_tmp;
				floor = graphs_tmp[i].floor_z;
				for (int k = 0; k < graphs_tmp[0].vertex_num; ++k)
					path[k] = -1;
				memcpy(path, parent, sizeof(parent));
			}

		}else
		{	
#endif		
			char* visited = (char*)malloc(sizeof(char) * vertex_n);
			memset(visited, 0, sizeof(char) * vertex_n);
			int tmp = 0, flag = 0;
			// steps_num_tmp = 0;
			//此次bfs的队列,并初始化！
			struct _queue q;
			q.data = (int*)malloc(sizeof(int) * vertex_n);
			for (int j = 0; j < vertex_n; ++j)
				q.data[j] = 0;
			q.capacity = vertex_n;
			q.front = 0; q.rear = 0;
			//此次bfs路径的暂存数组,初始化为-1！
			int parent[vertex_n];
			for (int j = 0; j < vertex_n; ++j)
				parent[j] = -1;
			visited[src] = 1;
			InsertQueue(&q, src);
		    while(q.front != q.rear && flag == 0)
		    {
		        tmp = DeleteQueue(&q);
#ifdef GOOD_BFS_IMPROVED 
		        if (map->all_path[tmp][des] != des)//说明在全局路径中找到了
		        {
		        	int src_tmp = tmp;
					while (src_tmp != des)
					{
						int node_tmp = map->all_path[src_tmp][des];
						parent[node_tmp] = src_tmp;
						src_tmp = node_tmp;
					}fine_some_route_cnt++;
					break;
		        }else    
#endif    	
		        // printf("des:%d,正在遍历顶点%d", des, tmp);
		        for(int j = 0; j < vertex_n; j++)
		        {
		            if( graphs_tmp[i].adj[j][tmp] != 0 && graphs_tmp[i].adj[tmp][j] != 0
		            	&& visited[j] == 0)
		            {
		            	parent[j] = tmp;
		                visited[j] = 1;
		                if(j == des) 
		                {
		            		flag = 1;break;
						}
						InsertQueue(&q, j);
		            }
		        }
				// printf(" flag=%d ", flag);
		        // PrintQueue(&q);
		    }
		    // for (int j = 0; j < vertex_n; ++j)
		    	// printf("[%d]%d\n", j, parent[j]);
#ifdef GOOD_BFS_IMPROVED 
		    for (int cnt = 0; cnt < vertex_n; ++cnt)
		    {
		    	int cnt_tmp = cnt;
			    while(parent[cnt_tmp] != -1)
			    {
			    	map->all_path[parent[cnt_tmp]][cnt] = cnt_tmp;
			    	cnt_tmp = parent[cnt_tmp];
				}
		    }
#endif		    
		    //每一层找到目标点之后
		    int j = des;
		    while(parent[j] != -1)
		    {
// #ifdef GOOD_BFS_IMPROVED 
// 		    	map->all_path[parent[j]][des] = j;
// #endif
		    	// printf("%d <- ", j);
		    	steps_num_tmp++;
		    	j = parent[j];
			}

			//找到真正的最小的路径，保存到path中！！
			steps_num_tmp += abs(graphs_tmp[i].floor_z - good->start_xyz[2]);//uav与此层差
			steps_num_tmp += abs(graphs_tmp[i].floor_z - good->end_xyz[2]);//目的地与此层差
			// printf("flor%d:%d\n", graphs_tmp[i].floor_z, steps_num_tmp);
			if (steps_num_tmp < steps_num )
			{
				steps_num = steps_num_tmp;
				floor = graphs_tmp[i].floor_z;
				for (int k = 0; k < graphs_tmp[0].vertex_num; ++k)
					path[k] = -1;
				memcpy(path, parent, sizeof(parent));
			}	
			//注意释放data的内存
			free(q.data);
			free(visited);
#ifdef GOOD_BFS_IMPROVED 
		}
#endif	
		if (HasBuilding(map, good->start_xyz[0], good->start_xyz[1], good->end_xyz[0], good->end_xyz[1]//如果两点之间都没障碍物，就不用算其他层啦
		, graphs_tmp[i].floor_z) )//后期再考虑两点之间的空间全是同一个building的情况！！！！
			break;
	}	
 	Good_GetPath(map, good, path, floor, des, src);
#ifdef GOOD_BFS_IMPROVED 
 	printf("find_full and some route cnt:%d, %d\n",find_full_route_cnt, fine_some_route_cnt);
#endif
	return 0;
}
/*************************************************
@Description: 得到搬运货物的最终路径
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int Good_GetPath(struct _map* map, struct _good* good, int* pr, int f_z, int des, int src)
{
	struct _map* map_tmp = map;
	struct _good* good_tmp = good;

	CDNT_NODE head_tmp = (ROUTE_LIST)malloc(sizeof(struct _node));
	//头节点是0！！！多余的
	for (int i = 0; i < 3; ++i)
		head_tmp->cdnt_xyz[i] = 0;
	head_tmp->next = NULL;
	CDNT_NODE node_tmp = head_tmp; //活动节点指针！！始终指向链表中最后一个节点

	//到此平面的路径！！！
	for (int j = good_tmp->start_xyz[2] + 1; j <= f_z; j++)
	{
		CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
    	tmp->cdnt_xyz[0] = good_tmp->start_xyz[0];
	    tmp->cdnt_xyz[1] = good_tmp->start_xyz[1];
	    tmp->cdnt_xyz[2] = j;  //z
	    tmp->next = NULL;
	    //更新活动节点指向！
	    node_tmp->next = tmp;
	    node_tmp = tmp;
	    good_tmp->transport_steps_num++;
	}

	//在此平面的路径！
	int i = des;
	while(pr[i] != -1) 
	{
	    //分配新节点,注意是倒着插入！！
	    CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	    tmp->cdnt_xyz[0] = i % map_tmp->battle_x;  //x
	    tmp->cdnt_xyz[1] = i / map_tmp->battle_x;  //y
	    tmp->cdnt_xyz[2] = f_z;  //z
	    //node_tmp不变！始终指向要插入的位置的前面一个
	    tmp->next = node_tmp->next;
	    node_tmp->next = tmp;
	    good_tmp->transport_steps_num++;

	    i = pr[i];
	}
	//将node_tmp指向链表末尾！
	while(node_tmp->next != NULL)
		node_tmp = node_tmp->next;
    //下降路径
	for (int j = f_z - 1; j >= good_tmp->end_xyz[2]; j--)
    {
    	CDNT_NODE tmp = (CDNT_NODE)malloc(sizeof(struct _node));
    	tmp->cdnt_xyz[0] = good_tmp->end_xyz[0];
	    tmp->cdnt_xyz[1] = good_tmp->end_xyz[1];
	    tmp->cdnt_xyz[2] = j;  //z
	    tmp->next = NULL;
	    //更新活动节点指向！
	    node_tmp->next = tmp;
	    node_tmp = tmp;
	    good_tmp->transport_steps_num++;
    }
    
	//注意有头结点
	good_tmp->transport_route->next = head_tmp->next;
	//头结点释放掉！
	free(head_tmp);
	return 0;
}
/*************************************************
@Description: 删除当前路径
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 此处map已经更新过了
*************************************************/
int DeleteTopNode(ROUTE_LIST l)
{
	CDNT_NODE tmp;
	tmp = l->next;
	l->next = tmp->next;
	free(tmp);

	return 0;
}

/*************************************************
@Description: 当前路径头节点插入一点
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int InsertTopNode(ROUTE_LIST l, const short x, const short y, const short z)
{
	CDNT_NODE tmp;
	tmp = (CDNT_NODE)malloc(sizeof(struct _node));
	tmp->cdnt_xyz[0] = x;
	tmp->cdnt_xyz[1] = y;
	tmp->cdnt_xyz[2] = z;
	tmp->next = NULL;

	CDNT_NODE node_tmp;
	node_tmp = l->next;
	l->next = tmp;
	tmp->next = node_tmp;

	return 0;
}
/*************************************************
@Description: 检测此三维空间是否有building存在
@Input: 
@Output: 
@Return: 0-存在   1-不存在  
@Others: 
*************************************************/
int HasBuilding(const struct _map* map, const short cx, const short cy, const short dx, const short dy, const short z)
{
	short z_tmp = (z < map->fly_low_limit ? map->fly_low_limit : z);
	short flag = 0;
	if (dy > cy)
	{
		for (int y = cy; y <= dy; ++y)
		{
			if (dx > cx)
				for (int x = cx; x <= dx; ++x)
					if((GetPointVal(map, x, y, z_tmp) & 0x3f) == 0x01)
					{
						flag = 1;
						break;
					}
			else
				for (int x = cx; x >= dx; --x)
					if((GetPointVal(map, x, y, z_tmp) & 0x3f) == 0x01)
					{
						flag = 1;
						break;
					}
			if (flag == 1) break;
		}
	}else
	{
		for (int y = cy; y >= dy; --y)
		{
			if (dx > cx)
				for (int x = cx; x <= dx; ++x)
					if((GetPointVal(map, x, y, z_tmp) & 0x3f) == 0x01)
					{
						flag = 1;
						break;
					}
			else
				for (int x = cx; x >= dx; --x)
					if((GetPointVal(map, x, y, z_tmp) & 0x3f) == 0x01)
					{
						flag = 1;
						break;
					}
			if (flag == 1) break;
		}
	}

	if (flag == 1)
		return 0;//如果找到障碍物
	else
		return 1;
}

/*************************************************
@Description: 入队
@Input: 
@Output: 
@Return: 0-成功   1-满了！！  
@Others: 
*************************************************/
int InsertQueue(struct _queue* Q, int x)
{
	if((Q->rear + 1) % Q->capacity == Q->front)
    	return 1;
    else
    {
        Q->data[Q->rear]=x;
        Q->rear = (Q->rear+1) % Q->capacity;
        return 0;
    }
}
/*************************************************
@Description: 出队
@Input: 
@Output: 
@Return: -1-失败   其他-队列头元素 
@Others:
*************************************************/
int DeleteQueue(struct _queue* Q)
{
    if(Q->front == Q->rear)
        return -1;
    else
    {
        int temp = Q->data[Q->front];
        Q->front = (Q->front + 1) % Q->capacity;
    	return temp;
    }	
}
/*************************************************
@Description: 
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
void PrintQueue(struct _queue* Q)
{
	printf("\nQueue:");
	for(int i = 0; i < Q->rear - Q->front; i++)
		printf("%d ",Q->data[i + Q->front]);
	printf("\n");
}
/*************************************************
@Description: 
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
void PrintGraphAdj(const struct _map* map, struct _graph* graph, short x, short y)
{
	int v = y * map->battle_x + x;
	//左
	if (v % map->battle_x != 0) //如果左边有点
	{
		printf("left:%d %d,", graph->adj[v][v - 1], graph->adj[v - 1][v]);
		//左上
		if (v + map->battle_x < graph->vertex_num) //如果上边有点
			printf("left-up:%d %d,", graph->adj[v][v + map->battle_x - 1], graph->adj[v + map->battle_x - 1][v]);
		//左下
		if (v - map->battle_x >= 0) //如果下边有点
			printf("left-down:%d %d,", graph->adj[v][v - map->battle_x - 1], graph->adj[v - map->battle_x - 1][v]);
	}
	//右
	if (v % map->battle_x != map->battle_x - 1) //如果右边有点
	{
		printf("right:%d %d,", graph->adj[v][v + 1], graph->adj[v + 1][v]);
		//右上
		if (v + map->battle_x < graph->vertex_num) //如果上边有点
			printf("right-up:%d %d,", graph->adj[v][v + map->battle_x + 1], graph->adj[v + map->battle_x + 1][v]);
		//右下
		if (v - map->battle_x >= 0) //如果xia边有点
			printf("right-down:%d %d,", graph->adj[v][v - map->battle_x + 1] , graph->adj[v - map->battle_x + 1][v]);
	}
			
	//上
	if (v + map->battle_x < graph->vertex_num) //如果上边有点
		printf("up:%d %d,", graph->adj[v][v + map->battle_x] , graph->adj[v + map->battle_x][v]);
	//下
	if (v - map->battle_x >= 0) //如果下边有点
		printf("down:%d %d,", graph->adj[v][v - map->battle_x] , graph->adj[v - map->battle_x][v]);
	printf("\n");
}
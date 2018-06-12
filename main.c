#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <unistd.h>//close、read、write函数需要
// #include <errno.h>
#include <fcntl.h>//F_GETFL设置非阻塞时需要
#include <sys/socket.h>
#include <sys/types.h>//数据类型定义
#include <arpa/inet.h>//ip地址转换

#include "json.h"
#include "tasks.h"
#include "route.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("argv num error!\n");
        return 1;
    }
	struct _client client;
    struct _map battle_map;
    struct _uav uavs[UAV_MAX_NUM];//以uav编号作为索引
    struct _good goods[GOOD_MAX_NUM]; //以good编号为索引
    struct _game_para game_paras;
#ifdef PRINT_COST_TIME
    long max_cost_time = 0, tim_tmp = 0;
    struct timeval stop, diff, start;  
    gettimeofday(&start, NULL);
#endif
    TaskInit(&game_paras, uavs, goods, &battle_map);
#ifdef PRINT_COST_TIME    
    gettimeofday(&stop, NULL);  
    printf("TaskInit总计用时:%ld微秒\n\n",timeval_subtract(&diff,&start,&stop)); 
#endif
    memcpy(game_paras.serv_ip, argv[1], strlen(argv[1]));
    game_paras.serv_port = atoi(argv[2]);
    memcpy(game_paras.token, argv[3], strlen(argv[3]));
    ClientInit(&client, game_paras.serv_ip, game_paras.serv_port);

    while(1) 
    {
    	int ret = PreProcess(&game_paras, &client, &battle_map, uavs, goods);
    	if (ret == 0)//连接关闭
    	{
    		// printf("client_socket %d close connection\n", client.clie_sock);
      //       close(client.clie_sock);
    	}else 
        if (ret == 3) //开始时刻为0时要上报初始位置
        {
#ifdef PRINT_COST_TIME
            struct timeval stop, diff, start;  
            gettimeofday(&start, NULL); 
#endif
            //提前进行分配计算！！！time 0
            DistributeTargets(&game_paras, &battle_map, uavs, goods);
            ReportInitInfos(&game_paras, &client, uavs);
#ifdef PRINT_COST_TIME
            gettimeofday(&stop, NULL);  
            printf("DistributeTargets + ReportInitInfos总计用时:%ld微秒\n\n",timeval_subtract(&diff,&start,&stop)); 
#endif
        }else
        if (ret == 4)//正常比赛了！
    	{
#ifdef PRINT_COST_TIME
            struct timeval stop, diff, start;  
            gettimeofday(&start, NULL);
#endif
    		DistributeTargets(&game_paras, &battle_map, uavs, goods);
#ifdef PRINT_COST_TIME
            gettimeofday(&stop, NULL);
            printf("DistributeTargets总计用时:%ld微秒\n",timeval_subtract(&diff,&start,&stop));
#endif 
			FlyUAVs(&game_paras, &client, &battle_map, uavs, goods);
#ifdef PRINT_COST_TIME
            gettimeofday(&stop, NULL);
            tim_tmp = timeval_subtract(&diff,&start,&stop);
            printf("DistributeTargets + FlyUAVs总计用时:%ld微秒\n\n", tim_tmp); 
            if(tim_tmp > max_cost_time)max_cost_time = tim_tmp;
#endif
    	}else
        if( ret == 5)//比赛结束!!
        {
#ifdef PRINT_COST_TIME
            printf("max_cost_time:%ld\n", max_cost_time);
#endif
            break;
        }
    }

    FreeMap(&battle_map);
    FreeGoodsInfo(goods);
    FreeUAVsInfo(uavs);
    return 0;
}
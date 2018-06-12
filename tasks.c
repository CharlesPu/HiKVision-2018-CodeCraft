/*******************************
@@Author     : Charles
@@Date       : 2018-05-07
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>


#include "tasks.h"
#include "json.h"
#include "config.h"
#include "route.h"


extern struct _uav_para* UAVs_para;
extern short* UAVs_para_id;
/*************************************************
@Description: 初始化所有task、各种全局变量
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 需要动态堆上分配的初始化对象不在此处，
        如map结构体的初始化，uavs_info结构体数组的初始化
*************************************************/
int TaskInit(struct _game_para* game_para, struct _uav* uavs, struct _good* good, struct _map* map)
{
    
    for (int i = 0; i < UAV_MAX_NUM; ++i)
    {
        uavs[i].aim_good = NULL;
        memset(uavs[i].aim_xyz, -1,sizeof(uavs[i].aim_xyz));
        memset(uavs[i].curr_xyz, -1,sizeof(uavs[i].curr_xyz));
        //初始化一个head节点
        uavs[i].route = (CDNT_NODE)malloc(sizeof(struct _node));
        memset(uavs[i].route->cdnt_xyz, 0, sizeof(uavs[i].route->cdnt_xyz));
        uavs[i].route->next = NULL;

        uavs[i].good_id = -1;//-1表示没有目标或者携带
        uavs[i].remain_electricity = 0;
        uavs[i].status = UAV_STAT_DESTROYED;
        uavs[i].wait_time = 0;
        uavs[i].uav_para = NULL;
    }
    for (int i = 0; i < GOOD_MAX_NUM; ++i)
    {
        good[i].id = i;
        memset(good[i].start_xyz, -1, sizeof(good[i].start_xyz));
        memset(good[i].end_xyz, -1, sizeof(good[i].end_xyz));
        good[i].weight = 0;
        good[i].value = 0;
        good[i].start_time = -1;
        good[i].remain_time = -1;
        good[i].status = GOOD_STAT_DISAPPEAR; //-1为不存在
        memset(good[i].aimed_uavs, 0, sizeof(good[i].aimed_uavs));
        good[i].aimed_uav_id = -1;
        good[i].aimed_uavs_num = 0;
        //初始化一个head节点
        good[i].transport_route = (CDNT_NODE)malloc(sizeof(struct _node));
        memset(good[i].transport_route->cdnt_xyz, 0, sizeof(good[i].transport_route->cdnt_xyz));
        good[i].transport_route->next = NULL;
        good[i].transport_steps_num = 0;
    }
    memset(game_para->user_id, 0, sizeof(game_para->user_id));
    memset(game_para->room_id, 0, sizeof(game_para->room_id));
    memset(game_para->serv_ip, 0, sizeof(game_para->serv_ip));
    game_para->serv_port = 0;
    memset(game_para->token, 0, sizeof(game_para->token));
    game_para->battle_time = 0;
    game_para->we_value = 0;
    game_para->enemy_value = 0;
    game_para->uavs_num = 0;
    game_para->goods_num = 0;
    game_para->goods_real_num = 0;
    game_para->uavs_type_num = 0;
    game_para->uavs_real_num = 0;
    for (int i = 0; i < GOOD_MAX_NUM; ++i)
        game_para->goods_val_sort[i] = 0;
    for (int i = 0; i < UAV_TYPE_MAX_NUM; ++i)
        game_para->uavs_val_sort[i] = i;
    game_para->no_good_interval = 0;
    for (int i = 0; i < UAV_MAX_NUM; ++i)
        game_para->is_attack_uav[i] = 0;
    game_para->back_charge_uavs_num = 0;

//放弃了在此处初始化图，因为memset耗时太长!
    // unsigned long len = sizeof(struct _graph) * GRAPH_MAX_NUM;
    // map->graphs = (struct _graph*)malloc(len);
    //全为0 啊！！
    // for (int i = 0; i < GRAPH_MAX_NUM; ++i)
    // {
    //     map->graphs[i].vertex_num = 0;
    //     map->graphs[i].floor_z = 0;
    //     memset(map->graphs[i].adj, 0, sizeof(map->graphs[i].adj));
    // }
    // memset(map->graphs, 0, sizeof(struct _graph) * GRAPH_MAX_NUM);
    return 0;
}

/*************************************************
@Description: 数据预处理，改变maps等全局变量
@Input: client  map地图指针  所有uav数组指针
@Output: 
@Return: 0-连接异常关闭或者比赛结束连接关闭   
        1-第一次连接的接收  2-第一次连接返回身份验证结果
        3-对战开始通知  4-从时刻 1 开始正常返回战场信息
@Others: 
*************************************************/
int PreProcess(struct _game_para* game_paras, struct _client* client, struct _map* map, struct _uav* uavs, struct _good* goods)
{
    struct _game_para* game_paras_tmp = game_paras;
    struct _client* client_tmp = client;
    struct _map* map_tmp = map;
    struct _uav* uavs_tmp = uavs;
    struct _good* goods_tmp = goods;
    int ret = 0; //返回值
    int res = ClientRecv(client_tmp);

    // if (res <= 0)//如果连接异常关闭
    // {
    //     printf("client_socket %d close connection\n", client_tmp->clie_sock);
    //     // close(client_tmp->clie_sock);
    //     ret = 0;
    // }else 
    if (res > 0)//如果连接正常，则开始解析
    {
#ifdef PRINT_COST_TIME
        printf("has received, and start processing.........................................................");
        struct timeval start;  
        gettimeofday(&start, NULL); 
#endif         
        char json_len_str[9] = {0};
        strncpy(json_len_str, client_tmp->c_rx_buf, 8);
        int json_len = atoi(json_len_str); 
        char rx_buffer[32766]= {0};
        int posi = 0;
        posi = sprintf(rx_buffer, "%s", client_tmp->c_rx_buf);
        while (res != json_len + 8)//防止ji次才能读完全
        {
            res += ClientRecv(client_tmp);
            posi += sprintf(rx_buffer + posi, "%s", client_tmp->c_rx_buf);
        }
        // printf("buff:%s\n", rx_buffer);

        //新建对象，用于下面的所有解析
        json_object* new_json_obj = json_tokener_parse(rx_buffer + 8); //8是因为前8个字节之后mei有空格
        //解析服务器发过来的notice键的值
        json_object* json_notice = json_object_object_get(new_json_obj, "notice");
#ifdef PRINT_BATTLE_TIME_NOTICE
        printf("notice:%s ", json_object_to_json_string(json_notice));
#endif
        //如果是刚建立连接第一次接收
        if (!strncmp("\"token\"", json_object_to_json_string(json_notice), strlen("\"token\""))) 
        {
            json_object* json_obj_tmp = json_object_new_object();
            json_object_object_add(json_obj_tmp, "token", json_object_new_string(game_paras_tmp->token));
            json_object_object_add(json_obj_tmp, "action", json_object_new_string("sendtoken"));
            int length = strlen(json_object_to_json_string(json_obj_tmp));
            char* str = (char*)malloc(length + 8);
            char str_tmp[10] = {0};
            int posi = 0;
            MyItoa(length, str_tmp, 10);
            for (int i = 0; i < 8 - strlen(str_tmp); ++i)
                posi += sprintf(str + posi, "%s", "0");
            posi += sprintf(str + posi, "%s", str_tmp);
            sprintf(str + posi, "%s", json_object_to_json_string(json_obj_tmp));
 
            ClientSend(client_tmp, str, strlen(str));
#ifdef PRINT_COST_TIME
            struct timeval stop, diff;
            gettimeofday(&stop, NULL);  
            printf("token总计用时:%ld微秒\n\n",timeval_subtract(&diff,&start,&stop)); 
#endif
            free(str);
            //释放json_obj!!!
            json_object_put(json_obj_tmp);
            ret = 1; 
        }else 
        //如果是刚建立返回身份验证结果
        if (!strncmp("\"tokenresult\"", json_object_to_json_string(json_notice), strlen("\"tokenresult\""))) 
        {
            //得到我的唯一id和对战房间id   
        	json_object* json_val = json_object_object_get(new_json_obj, "yourId");
            memcpy(game_paras_tmp->user_id, json_object_to_json_string(json_val), strlen(json_object_to_json_string(json_val)));
            json_val = json_object_object_get(new_json_obj, "roundId");
            memcpy(game_paras_tmp->room_id, json_object_to_json_string(json_val), strlen(json_object_to_json_string(json_val)));
            //身份验证结果！
            json_val = json_object_object_get(new_json_obj, "result");
            int result = json_object_get_int(json_val);
            if (result == 0)//如果合法！
            {
                json_object* json_obj_tmp = json_object_new_object();    
                json_object_object_add(json_obj_tmp, "token", json_object_new_string(game_paras_tmp->token));
                json_object_object_add(json_obj_tmp, "action", json_object_new_string("ready"));

                int length = strlen(json_object_to_json_string(json_obj_tmp));
                char* str = (char*)malloc(length + 8);
                char str_tmp[10] = {0};
                int posi = 0;
                MyItoa(length, str_tmp, 10);
                for (int i = 0; i < 8 - strlen(str_tmp); ++i)
                    posi += sprintf(str + posi, "%s", "0");
                posi += sprintf(str + posi, "%s", str_tmp);
                sprintf(str + posi, "%s", json_object_to_json_string(json_obj_tmp));

                ClientSend(client_tmp, str, strlen(str));
#ifdef PRINT_COST_TIME
                struct timeval stop, diff;
                gettimeofday(&stop, NULL);                  
                printf("tokenresult总计用时:%ld微秒\n\n",timeval_subtract(&diff,&start,&stop)); 
#endif               
                free(str);
                //释放json_obj!!!
                json_object_put(json_obj_tmp);
            }else //如果非法！
            {

            }

            ret = 2;
        }else 
        //如果是对战开始通知
        if (!strncmp("\"sendmap\"", json_object_to_json_string(json_notice), strlen("\"sendmap\""))) 
        {
            //更新比赛时间
        	json_object* json_val = json_object_object_get(new_json_obj, "time");
            game_paras_tmp->battle_time = json_object_get_int(json_val); 
            //初始化地图信息和无人机参数
            json_val = json_object_object_get(new_json_obj, "map");
            MapInit(json_val, map_tmp);
            UAVsParaInit(json_val, game_paras_tmp); //此处改变了全局变量 UAVs_para
            //解析无人机信息！
            json_object* json_val_uav = json_object_object_get(json_val, "init_UAV");
            UpdateUAVsInfo(game_paras_tmp, json_val_uav, uavs_tmp);
#ifdef PRINT_COST_TIME
            struct timeval stop, diff;
            gettimeofday(&stop, NULL);             
            printf("sendmap总计用时:%ld微秒\n",timeval_subtract(&diff,&start,&stop)); 
#endif
            ret = 3;
        }else 
        //如果是从时刻 1 开始正常返回战场信息，解析战场所有数据！！！！
        if (!strncmp("\"step\"", json_object_to_json_string(json_notice), strlen("\"step\""))) 
        {
        	json_object* json_val = json_object_object_get(new_json_obj, "match_status");
            int match_stat = json_object_get_int(json_val);
            if (match_stat == 1) //比赛已经结束啦！！
            {
                printf("\ngame over! client_socket %d close connection\n", client_tmp->clie_sock);
                close(client_tmp->clie_sock);
#ifdef PRINT_BATTLE_TIME_NOTICE
                json_val = json_object_object_get(new_json_obj, "time");
                printf("time:%d\n", json_object_get_int(json_val));
#endif  
#ifdef PRINT_TOTAL_VALUE
                json_val = json_object_object_get(new_json_obj, "we_value");
                int we_val = json_object_get_int(json_val);printf("we_val:%d ", we_val);
                json_val = json_object_object_get(new_json_obj, "UAV_we");
                UpdateUAVsInfo(game_paras_tmp, json_val, uavs_tmp);
                int we_uav_val = 0;
                for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
                    if(uavs_tmp[i].status != UAV_STAT_DESTROYED)
                        we_uav_val += uavs_tmp[i].uav_para->value;
                printf("UAV_we:%d ", we_uav_val);
                printf("total_value:%d\n\n", we_val + we_uav_val);
                for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
                    printf("uav[%d]:stat %d, val %d, lw %d, cap %d, chag %d, re_ele %d\n", i, uavs_tmp[i].status
                        , uavs_tmp[i].uav_para->value, uavs_tmp[i].uav_para->load_weight
                        , uavs_tmp[i].uav_para->capacity, uavs_tmp[i].uav_para->charge, uavs_tmp[i].remain_electricity);
#endif
                ret = 5;
            }else
            {   
                //更新比赛时间
                json_val = json_object_object_get(new_json_obj, "time");
                game_paras_tmp->battle_time = json_object_get_int(json_val);
#ifdef PRINT_BATTLE_TIME_NOTICE
                printf("time:%d\n", game_paras_tmp->battle_time);
#endif
                //敌我双方总价值
                json_val = json_object_object_get(new_json_obj, "we_value");
                game_paras_tmp->we_value = json_object_get_int(json_val);
                json_val = json_object_object_get(new_json_obj, "enemy_value");
                game_paras_tmp->enemy_value = json_object_get_int(json_val);
                //我方无人机信息，货物信息，地方无人机map标出1
                //一定要在更新无人机信息之前，因为uav目标good可能已失效
                UpdateGoodsInfo(game_paras_tmp, new_json_obj, goods_tmp);
                json_val = json_object_object_get(new_json_obj, "UAV_we");
                UpdateUAVsInfo(game_paras_tmp, json_val, uavs_tmp);
                UpdateMap(json_val, map_tmp, uavs_tmp, goods_tmp, 1);
                json_val = json_object_object_get(new_json_obj, "UAV_enemy");
                UpdateMap(json_val, map_tmp, uavs_tmp, goods_tmp, -1); 
#ifdef PRINT_COST_TIME
                struct timeval stop, diff;
                gettimeofday(&stop, NULL);              
                printf("step总计用时:%ld微秒\n",timeval_subtract(&diff,&start,&stop)); 
#endif
                ret = 4; 
            }
        }
        //注意释放，与开头配对使用
        json_object_put(new_json_obj);
    }
    
    return ret;
}

/*************************************************
@Description: 为每个uav分配合适的目标good！
@Input: 比赛参数  map  uavs数组   货物数组
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int DistributeTargets(struct _game_para* game_paras, struct _map* map, struct _uav* uavs, struct _good* goods)
{
    struct _game_para* game_paras_tmp = game_paras;
    struct _map* map_tmp = map;
    struct _uav* uavs_tmp = uavs;
    struct _good* goods_tmp = goods;
    //先分配good给uav去抢！
    game_paras_tmp->goods_real_num = 0;
    int bfs_loop_cnt = 0;
// #pragma omp parallel for 
//     for (int i = 0; i < game_paras_tmp->goods_num; ++i)
//     {   //如果是需要规划搬运路径的空闲good
//         int id_tmp = game_paras_tmp->goods_val_sort[i];//printf("start%d ", id_tmp);
//         if (!IsGoodFree(goods_tmp[id_tmp]) && goods_tmp[id_tmp].transport_route->next == NULL && bfs_loop_cnt < 2)
//         {
// #ifdef PRINT_BFS
//             printf("bfs_good[%d]start...  \n", id_tmp);
// #endif
//             Good_BFS(game_paras_tmp, map_tmp, map_tmp->graphs, &(goods_tmp[id_tmp]));
//             bfs_loop_cnt++;
// #ifdef PRINT_BFS
//             printf("bfs_good[%d]end\n", id_tmp);
// #endif            
//         }
//     }
#ifdef PRINT_CHARGE_NUM
    printf("homestat:%d+++++++++++++++++++++++++\n", map_tmp->home_state);
#endif
    game_paras_tmp->no_good_interval = (game_paras_tmp->no_good_interval > 1000) ? 1000 : game_paras_tmp->no_good_interval + 1;
    for (int i = 0; i < game_paras_tmp->goods_num; ++i)
    {  
        // printf("gno[%d]-auvnum%d-stat%d\n", i, goods_tmp[i].aimed_uavs_num, goods_tmp[i].status);
        int id_tmp = game_paras_tmp->goods_val_sort[i];
        if (!IsGoodFree(goods_tmp[id_tmp]) )
        {   
            game_paras_tmp->no_good_interval = 0;//有人不适合了说明有空闲good了
#ifdef PRINT_DISTRIBUTION
            printf("distributing good[%d]: wei %d ,val %d, tr_steps_num %d \n"
                , goods_tmp[id_tmp].id
                , goods_tmp[id_tmp].weight, goods_tmp[id_tmp].value
                , goods_tmp[id_tmp].transport_steps_num);
#endif
            game_paras_tmp->goods_real_num++;
            //开始寻找最佳匹配的uav
            int uav_id = -1;
            int stblt = 0;
            for (int j = 0; j < game_paras_tmp->uavs_num; ++j)
            {
                if (!IsUAVFree(uavs_tmp[j]))
                {
                    //已经要到家充电的uav不能分配，防止堵住他人去路
                    if (  game_paras_tmp->is_attack_uav[j] == 1 //或者已经是攻击机了
                        || (uavs_tmp[j].aim_xyz[0] == map_tmp->parking_x //如果是要回去充电的uav，
                            && uavs_tmp[j].aim_xyz[1] == map_tmp->parking_y 
                            && uavs_tmp[j].aim_xyz[2] == 0  
                            && CalculateDistancePow2(uavs_tmp[j].curr_xyz[0], uavs_tmp[j].curr_xyz[1]//如果是在老家上面附近了！就不要分配了，防止变动挡住别人去路
                            , map_tmp->parking_x, map_tmp->parking_y) <= MAP_STAT_JUDGE_DIS_POW))
                            continue;
                    unsigned int stblt_tmp = CalculateSuitability(&(goods_tmp[id_tmp]), &(uavs_tmp[j]));
#ifdef PRINT_DISTRIBUTION
                    printf("uav[%d]: lw %d, cap %d, chag %d, re_ele %d, stbl:%d\n", j, uavs_tmp[j].uav_para->load_weight
                        , uavs_tmp[j].uav_para->capacity, uavs_tmp[j].uav_para->charge
                        , uavs_tmp[j].remain_electricity, stblt_tmp);
#endif
                    if (stblt_tmp > stblt)
                    {
                        stblt = stblt_tmp;
                        uav_id = j;
                    }
                }
            }
            if (stblt !=0 && uav_id != -1)//如果分配到了uav
            {
#ifdef PRINT_DISTRIBUTION
                printf("choose uav%d\n", uav_id);
#endif
                //更新uav
                //如果是本来回家充电的，要更新统计数
                if (   uavs_tmp[uav_id].aim_xyz[0] == map_tmp->parking_x //如果是要回去充电的uav，
                    && uavs_tmp[uav_id].aim_xyz[1] == map_tmp->parking_y 
                    && uavs_tmp[uav_id].aim_xyz[2] == 0 )
                {
                    game_paras_tmp->back_charge_uavs_num--;
#ifdef PRINT_CHARGE_NUM
                    printf("uav%d cha->good\n", uav_id);
#endif
                }
                    
                uavs_tmp[uav_id].aim_good   = &(goods_tmp[id_tmp]);
                uavs_tmp[uav_id].aim_xyz[0] = goods_tmp[id_tmp].start_xyz[0];
                uavs_tmp[uav_id].aim_xyz[1] = goods_tmp[id_tmp].start_xyz[1];
                uavs_tmp[uav_id].aim_xyz[2] = goods_tmp[id_tmp].start_xyz[2];
                FreeRoute(uavs_tmp[uav_id].route);
                //更新good
                goods_tmp[id_tmp].aimed_uavs[uav_id] = 1;
                goods_tmp[id_tmp].aimed_uav_id  = uav_id;
                goods_tmp[id_tmp].aimed_uavs_num++;
                game_paras_tmp->is_attack_uav[uav_id] = 0; 
                game_paras_tmp->goods_real_num--;
            }
        }
    }
//如果是上面刚分配过uav的good，要检查是否是在没有bfs过情况下分配的
#pragma omp parallel for 
    for (int i = 0; i < game_paras_tmp->goods_num; ++i)
    {   
        int id_tmp = game_paras_tmp->goods_val_sort[i];//printf("start%d ", id_tmp);
        if (goods_tmp[id_tmp].aimed_uavs_num && goods_tmp[id_tmp].aimed_uav_id != -1 && goods_tmp[id_tmp].status == GOOD_STAT_NORMAL && bfs_loop_cnt < GRAPH_GOOD_BFS_LOOP_MAX)
        {
            if(goods_tmp[id_tmp].transport_route->next == NULL)//如果真的是没有过bfs，那么要bfs，并重新检查电量的匹配度
            {
#ifdef PRINT_BFS
                printf("bfs_good[%d]start...  \n", id_tmp);
#endif
                Good_BFS(game_paras_tmp, map_tmp, map_tmp->graphs, &(goods_tmp[id_tmp]));
                int uav_id_tmp = goods_tmp[id_tmp].aimed_uav_id;
#ifdef PRINT_DISTRIBUTION    
                printf("uav_ele:%d, need_ele:%d, tr_step:%d\n", uavs_tmp[uav_id_tmp].remain_electricity
                    , goods_tmp[id_tmp].weight * (goods_tmp[id_tmp].transport_steps_num + GOOD_TRANS_MARGIN), goods_tmp[id_tmp].transport_steps_num);
#endif
                if (uavs_tmp[uav_id_tmp].remain_electricity < goods_tmp[id_tmp].weight * (goods_tmp[id_tmp].transport_steps_num + GOOD_TRANS_MARGIN))//如果电量不够搬运的话   +1是因为刚下去取到good时也会耗电
                {
#ifdef PRINT_DISTRIBUTION
                    printf("good%d cancel choose uav%d\n", id_tmp, uav_id_tmp);
#endif
                    game_paras_tmp->no_good_interval = 0;//有人不适合了说明有空闲good了
                    goods_tmp[id_tmp].aimed_uavs_num--;
                    goods_tmp[id_tmp].aimed_uav_id = -1;
                    goods_tmp[id_tmp].aimed_uavs[uav_id_tmp] = 0;
                    uavs_tmp[uav_id_tmp].aim_good = NULL;
                    uavs_tmp[uav_id_tmp].aim_xyz[0] = -1;
                    uavs_tmp[uav_id_tmp].aim_xyz[1] = -1;
                    uavs_tmp[uav_id_tmp].aim_xyz[2] = -1;

                    game_paras_tmp->goods_real_num++;
                }
                bfs_loop_cnt++;
#ifdef PRINT_BFS
                printf("bfs_good[%d]end\n", id_tmp);
#endif            
            }
        }
    }

    //再修改每个uav的动作！
    for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
    {   
        if (!IsUAVFree(uavs_tmp[i]))
        {   
            int dis_pow2_tmp = CalculateDistancePow2(uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1]
                    , map_tmp->parking_x, map_tmp->parking_y);
            //已经要到家充电的uav不能分配，防止堵住他人去路
            if ( game_paras_tmp->is_attack_uav[i] == 1//或者已经是攻击机了  
                ||  (  uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x //如果是要回去充电的uav，
                    && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y 
                    && uavs_tmp[i].aim_xyz[2] == 0  
                    && dis_pow2_tmp <= MAP_STAT_JUDGE_DIS_POW))//如果是在老家上面附近了！就不要分配了，防止变动挡住别人去路
                    continue;
            int s = 0;//CountAttackUAVNums(game_paras_tmp, uavs_tmp);
            //如果能做攻击机
            if (game_paras_tmp->no_good_interval > UAV_ATTACK_DISTRIBUTE_INTERVAL && s < UAV_ATTACK_MAX_NUM && game_paras_tmp->uavs_real_num > UAV_ATTACK_MAX_NUM
                && uavs_tmp[i].uav_para->value == UAVs_para[game_paras_tmp->uavs_val_sort[1]].value && game_paras_tmp->is_attack_uav[i] == 0)
            {
                if (   uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x //如果是要回去充电的uav，
                    && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y 
                    && uavs_tmp[i].aim_xyz[2] == 0 )
                {
                     game_paras_tmp->back_charge_uavs_num--;
#ifdef PRINT_CHARGE_NUM
                     printf("uav%d cha->attack\n",i);
#endif
                } 
                uavs_tmp[i].aim_xyz[0] = map_tmp->enemy_parking_x;
                uavs_tmp[i].aim_xyz[1] = map_tmp->enemy_parking_y;
                uavs_tmp[i].aim_xyz[2] = 1;
                FreeRoute(uavs_tmp[i].route);
                game_paras_tmp->is_attack_uav[i] = 1; 
            }else//如果正常飞机
            {
                //如果电量没满,并且要在外面！！让它回家！!!不要轻易修改其目标点,防止是在家里想去待命区的uav
                if (   game_paras_tmp->back_charge_uavs_num < UAV_BACK_CHARGE_MAX_NUM 
                    && uavs_tmp[i].remain_electricity < uavs_tmp[i].uav_para->capacity
                    && dis_pow2_tmp > MAP_STAT_JUDGE_DIS_POW)
                {
                    if (!( uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x
                        && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y
                        && uavs_tmp[i].aim_xyz[2] == 0))
                    {
                        uavs_tmp[i].aim_xyz[0] = map_tmp->parking_x;
                        uavs_tmp[i].aim_xyz[1] = map_tmp->parking_y;
                        uavs_tmp[i].aim_xyz[2] = 0;
                        game_paras_tmp->is_attack_uav[i] = 0;
                        game_paras_tmp->back_charge_uavs_num++;
#ifdef PRINT_CHARGE_NUM
                        printf("###############back_charge id %d, re_ele %d, num %d\n", i, uavs_tmp[i].remain_electricity, game_paras_tmp->back_charge_uavs_num);
#endif
                    }
                }else//要么满电，要么在家里时
                if (uavs_tmp[i].route->next == NULL)    
                {
                    if (game_paras_tmp->no_good_interval > UAV_BACK_DISTRIBUTE_INTERVAL )
                    {
                        uavs_tmp[i].aim_xyz[0] = map_tmp->parking_x;
                        uavs_tmp[i].aim_xyz[1] = map_tmp->parking_y;
                        uavs_tmp[i].aim_xyz[2] = 0;
                        game_paras_tmp->is_attack_uav[i] = 0;
                    }else
                    {//去待命点！
                        int prio_num = pow(2, (map_tmp->battle_x > map_tmp->battle_y ? map_tmp->battle_y : map_tmp->battle_x) / 4);
                        prio_num = (prio_num < map_tmp->battle_x * map_tmp->battle_y ? prio_num : map_tmp->battle_x * map_tmp->battle_y / 2);
                        int standby_xy = map_tmp->standby[rand() % prio_num];
                        int standby_x = standby_xy % map_tmp->battle_x;
                        int standby_y = standby_xy / map_tmp->battle_x;
                        int loop_cnt = 0, floor_add = 0;//防止找遍了地图所有点
                        //待命区全部在最低高度以上，防止堵住下行拿货物
                        while(loop_cnt < map_tmp->battle_x * map_tmp->battle_y
                            && (GetPointVal(map_tmp, standby_x, standby_y, map_tmp->fly_low_limit + floor_add) & 0x3f) != 0x00) 
                        {
                            if (loop_cnt >= map_tmp->battle_x * map_tmp->battle_y)
                            {
                                loop_cnt = 0;
                                floor_add++;//如果这一层找完了，就找上一层的！
                            }
                            if (loop_cnt >= prio_num)//如果优先找的被找完了，就找数组中剩下的
                                standby_xy = map_tmp->standby[map_tmp->battle_x * map_tmp->battle_y -1 - rand() % prio_num];
                            else
                                standby_xy = map_tmp->standby[rand() % prio_num];
                            standby_x = standby_xy % map_tmp->battle_x;
                            standby_y = standby_xy / map_tmp->battle_x;
                            loop_cnt++;   
                        }
                        uavs_tmp[i].aim_xyz[0] = standby_x;
                        uavs_tmp[i].aim_xyz[1] = standby_y;
                        uavs_tmp[i].aim_xyz[2] = map_tmp->fly_low_limit + floor_add;
                        game_paras_tmp->is_attack_uav[i] = 0; 
                    } 
                } 
            }       
        }
    }
#ifdef PRINT_BFS
    printf("bfs_loop_cnt:%d\n", bfs_loop_cnt);
#endif
    //再最终开始BFS大法！
#pragma omp parallel for 
    for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
    {   
        if (!IsUAVNeedPlan(uavs_tmp[i]) && bfs_loop_cnt < GRAPH_UAV_BFS_LOOP_MAX)
        {
#ifdef PRINT_BFS
            printf("bfs_uav[%d]start ",i);
            if (uavs_tmp[i].aim_good != NULL)
                printf("aim_good[%d].value:%d ", uavs_tmp[i].aim_good->id, uavs_tmp[i].aim_good->value);
            printf("\n");
#endif
            BFS(game_paras_tmp, map_tmp, map_tmp->graphs, &(uavs_tmp[i]));
            bfs_loop_cnt++;
#ifdef PRINT_BFS
            printf("bfs_uav[%d]end\n",i);
#endif
        }
    }
    //     for (int i = 0; i < game_paras_tmp->uavs_real_num; ++i)
    // {
    //     printf("cao: %d : %d\n", i, game_paras_tmp->is_attack_uav[i]);
    // }   
    return 0;
}

/*************************************************
@Description: 开始移动
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int FlyUAVs(struct _game_para* game_paras, struct _client* client, struct _map* map, struct _uav* uavs, struct _good* goods)
{
    struct _game_para* game_paras_tmp = game_paras;
    struct _client* client_tmp = client;
    struct _map* map_tmp = map;
    struct _uav* uavs_tmp = uavs;
    struct _good* goods_tmp = goods;

    json_object* new_json_obj = json_object_new_object(); 
    json_object_object_add(new_json_obj, "token", json_object_new_string(game_paras_tmp->token));
    json_object_object_add(new_json_obj, "action", json_object_new_string("flyPlane"));
    json_object *json_obj_array = json_object_new_array();  

    for (int i = 0; i < game_paras_tmp->uavs_num; ++i) 
    {
        if (uavs_tmp[i].status != UAV_STAT_DESTROYED)//每一个有效uav
        { 
            // printf("uav[%d]:", i);if(uavs_tmp[i].route->next != NULL)PrintRoute(uavs_tmp[i].route);
            // printf("aim x:%d,y:%d\n", uavs_tmp[i].aim_xyz[0],uavs_tmp[i].aim_xyz[1]);
            int x = 0, y = 0, z = 0;
            if (uavs_tmp[i].route == NULL || uavs_tmp[i].route->next == NULL)//防止bug出现
            {
                //等待！
#ifdef PRINT_WAIT_CAUSE
                printf("uav%d wait: no route, wait_time%d, is_attack%d\n", i, uavs_tmp[i].wait_time, game_paras_tmp->is_attack_uav[i]);
#endif
                x = uavs_tmp[i].curr_xyz[0];
                y = uavs_tmp[i].curr_xyz[1];
                z = uavs_tmp[i].curr_xyz[2];
                // printf("good_id%d,x%dy%dz%d\n", uavs_tmp[i].good_id,uavs_tmp[i].aim_xyz[0], uavs_tmp[i].aim_xyz[1],uavs_tmp[i].aim_xyz[2]);
            }else//有待走的路径
            {
                //判断有没有uav挡着下一步了
                unsigned char val_next = GetPointVal(map_tmp, uavs_tmp[i].route->next->cdnt_xyz[0], 
                    uavs_tmp[i].route->next->cdnt_xyz[1], uavs_tmp[i].route->next->cdnt_xyz[2]);
                unsigned char val_curr = GetPointVal(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2]);
                if ( (val_next & 0x3f) == 0x00)//如果下一步没有uav
                {
                    int dis_pow2_tmp = CalculateDistancePow2(uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1]
                            , map_tmp->parking_x, map_tmp->parking_y);
                    //先判断是否是需要停顿的回家充电uav
                    if (   uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x //如果是要回去充电的uav，
                        && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y 
                        && uavs_tmp[i].aim_xyz[2] == 0  
                        && map_tmp->home_state    == HOME_STAT_OUT  //且家里有人要出来！
                        && dis_pow2_tmp <= MAP_CHARGE_WAIT_DIS_POW && dis_pow2_tmp > MAP_STAT_JUDGE_DIS_POW)//且距离家已经很近了！已经在家的不算！
                    {    //等待！
#ifdef PRINT_WAIT_CAUSE
                        printf("uav%d wait: back home charge, wait_time%d, is_attack%d\n", i, uavs_tmp[i].wait_time, game_paras_tmp->is_attack_uav[i]);
#endif
                        x = uavs_tmp[i].curr_xyz[0];
                        y = uavs_tmp[i].curr_xyz[1];
                        z = uavs_tmp[i].curr_xyz[2];
                    }else//如果从家准备出来的uav需要停顿！
                    if (   uavs_tmp[i].curr_xyz[0] == map_tmp->parking_x 
                        && uavs_tmp[i].curr_xyz[1] == map_tmp->parking_y
                        && uavs_tmp[i].curr_xyz[2] == 0
                        && (map_tmp->home_state == HOME_STAT_IN //家里有人要进来！！
                            //或者 没有目标good或者有目标good但是电不够的的非攻击机，没满电也要停顿,不让其直接去待命区!!!!
                            || ((uavs_tmp[i].aim_good == NULL 
                                    || (uavs_tmp[i].aim_good != NULL //此情况是因为开场时的good分配，提前计算bfs的
                                        && uavs_tmp[i].remain_electricity < uavs_tmp[i].aim_good->weight * (uavs_tmp[i].aim_good->transport_steps_num + GOOD_TRANS_MARGIN)))
                                && game_paras_tmp->is_attack_uav[i] == 0
                                && uavs_tmp[i].remain_electricity != uavs_tmp[i].uav_para->capacity)))
                    {   //等待！
#ifdef PRINT_WAIT_CAUSE
                        printf("uav%d wait: out home, wait_time%d, is_attack%d\n", i, uavs_tmp[i].wait_time, game_paras_tmp->is_attack_uav[i]);
#endif
                        x = uavs_tmp[i].curr_xyz[0];
                        y = uavs_tmp[i].curr_xyz[1];
                        z = uavs_tmp[i].curr_xyz[2];
                    }else//往前走！
                    { 
                        x = uavs_tmp[i].route->next->cdnt_xyz[0];
                        y = uavs_tmp[i].route->next->cdnt_xyz[1];
                        z = uavs_tmp[i].route->next->cdnt_xyz[2];
                        uavs_tmp[i].wait_time = 0;
                        /***********************************改变老家状态！************************************/
                        if (   uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x //如果是回家充电的
                            && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y 
                            && uavs_tmp[i].aim_xyz[2] == 0
                            && dis_pow2_tmp <= MAP_STAT_JUDGE_DIS_POW) 
                        {
                            map_tmp->home_state = HOME_STAT_IN;
#ifdef PRINT_CHARGE_NUM
                            printf("change home state>2 uav_id:%d--------------\n", i);
#endif
                        }else//此情况包括路过老家上方的uav的干扰，这样也好，警惕些。
                        if (   dis_pow2_tmp <= MAP_STAT_JUDGE_DIS_POW
                            && !(  uavs_tmp[i].aim_xyz[0] == map_tmp->parking_x //and not回家充电的
                                && uavs_tmp[i].aim_xyz[1] == map_tmp->parking_y 
                                && uavs_tmp[i].aim_xyz[2] == 0)) 
                        {
                            map_tmp->home_state = HOME_STAT_OUT;
#ifdef PRINT_CHARGE_NUM
                            printf("change home state>1 uav_id:%d--------------\n", i);
#endif
                        }
                        //删除路径的当前点！
                        DeleteTopNode(uavs_tmp[i].route);
                        //交叉走处理！
                        if (uavs_tmp[i].curr_xyz[2] == z)//往上下走的跳过判断!
                        {
                            char val_left  = GetPointVal(map_tmp, uavs_tmp[i].curr_xyz[0], y, z);
                            char val_right = GetPointVal(map_tmp, x, uavs_tmp[i].curr_xyz[1], z);
                            //左右路径都设为不能走,设为障碍物
                            if ((val_left & 0x3f)  == 0x00)
                                SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], y, z, val_left | 0X3f); 
                            if ((val_right & 0x3f) == 0x00)
                                SetMapValue(map_tmp, x, uavs_tmp[i].curr_xyz[1], z, val_right | 0x3f); 
                        }    
                        unsigned char level = (uavs_tmp[i].uav_para->value) / UAV_VAL_CVT_BASE;
                        SetMapValue(map_tmp, x, y, z, val_next | (level & 0x3f));     
                        SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2], val_curr & 0x40);
                        //刚拿到货物!                    
                        if (uavs_tmp[i].route->next == NULL && uavs_tmp[i].aim_good != NULL 
                            && x == uavs_tmp[i].aim_xyz[0] && y == uavs_tmp[i].aim_xyz[1] 
                            && z == uavs_tmp[i].aim_xyz[2])
                        {
                            if (uavs_tmp[i].aim_good->left_time > 0 && uavs_tmp[i].aim_good->status == GOOD_STAT_NORMAL)//拿到货物了！！
                            {
                                uavs_tmp[i].aim_xyz[0] = uavs_tmp[i].aim_good->end_xyz[0];
                                uavs_tmp[i].aim_xyz[1] = uavs_tmp[i].aim_good->end_xyz[1];
                                uavs_tmp[i].aim_xyz[2] = uavs_tmp[i].aim_good->end_xyz[2];        
                                uavs_tmp[i].good_id = uavs_tmp[i].aim_good->id;
                                /***********电量修改！************/
                                // int ele_tmp = uavs_tmp[i].remain_electricity - uavs_tmp[i].aim_good->weight;
                                // uavs_tmp[i].remain_electricity = ( ele_tmp <= 0 ? 0 : ele_tmp);
                                //直接给此uav路径！！
                                uavs_tmp[i].route->next = uavs_tmp[i].aim_good->transport_route->next;
                                uavs_tmp[i].aim_good->transport_route->next = NULL;

                            }else//若刚好不能拿了，就放弃
                            {
                                // memset(uavs_tmp[i].aim_xyz, -1, sizeof(uavs_tmp[i].aim_xyz));//清空目标good坐标
                                uavs_tmp[i].aim_xyz[0] = -1;
                                uavs_tmp[i].aim_xyz[1] = -1;
                                uavs_tmp[i].aim_xyz[2] = -1;  
                                uavs_tmp[i].good_id = -1;
                                FreeRoute(uavs_tmp[i].route);//清空路径！
                            }     
                            uavs_tmp[i].aim_good->aimed_uavs[i] = 0;
                            uavs_tmp[i].aim_good->aimed_uavs_num = 0;
                            uavs_tmp[i].aim_good = NULL;//清空其目标good
                        }else//如果正好到家了
                        if (x == map_tmp->parking_x && y == map_tmp->parking_y && z == 0)
                        {
                            uavs_tmp[i].aim_xyz[0] = -1;
                            uavs_tmp[i].aim_xyz[1] = -1;
                            uavs_tmp[i].aim_xyz[2] = -1;  
                            game_paras_tmp->back_charge_uavs_num--;
#ifdef PRINT_CHARGE_NUM
                            printf("uav%d cha->at home\n", i);
#endif
                        }
                        /************************************刚回到家准备充电**************************************/
                        // if (uavs_tmp[i].route->next == NULL && uavs_tmp[i].aim_good == NULL 
                        //     && x == map_tmp->parking_x && y == map_tmp->parking_y && z == 0)
                        // {
                        //     int ele_tmp = uavs_tmp[i].remain_electricity + uavs_tmp[i].uav_para->charge;
                        //     uavs_tmp[i].remain_electricity = ( ele_tmp > uavs_tmp[i].uav_para->capacity ? uavs_tmp[i].uav_para->capacity : ele_tmp);
                        // }
                    }
                }else //如果下一步有uav
                {
                    //如果是敌方
                    if ((val_next & 0x80) == 0x80 )
                    {
                        if ( ((val_next & 0x3f) > (val_curr & 0x3f)) )//且总机价值等级比我的总机价值等级大，就撞他
                        {
                            //往前走！
                            x = uavs_tmp[i].route->next->cdnt_xyz[0];
                            y = uavs_tmp[i].route->next->cdnt_xyz[1];
                            z = uavs_tmp[i].route->next->cdnt_xyz[2];
                            uavs_tmp[i].wait_time = 0;
                            //删除路径的当前点！
                            DeleteTopNode(uavs_tmp[i].route);
                            unsigned char level = (uavs_tmp[i].uav_para->value) / UAV_VAL_CVT_BASE;
                            SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2], val_curr & 0x40);
                            SetMapValue(map_tmp, x, y, z, val_next | (level & 0x3f));
                        }else//如果我比他大
                        {
                            if (uavs_tmp[i].curr_xyz[2] == uavs_tmp[i].route->next->cdnt_xyz[2])//而且本来不是上下走的,赶紧上下躲开
                            {
#ifdef PRINT_WAIT_CAUSE
                                printf("uav%d wait: enemey bigger, wait_time%d, is_attack%d\n", i, uavs_tmp[i].wait_time, game_paras_tmp->is_attack_uav[i]);
#endif
                                x = uavs_tmp[i].curr_xyz[0];
                                y = uavs_tmp[i].curr_xyz[1];
                                z = uavs_tmp[i].curr_xyz[2];
                                uavs_tmp[i].wait_time++;if (uavs_tmp[i].wait_time > 30000)uavs_tmp[i].wait_time = 0;
                                unsigned char val_next_tmp = GetPointVal(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], 
                                                (uavs_tmp[i].curr_xyz[2] + 1 > map_tmp->fly_high_limit ? 
                                                (uavs_tmp[i].curr_xyz[2] - 1) : (uavs_tmp[i].curr_xyz[2] + 1)));
                                if ((val_next_tmp & 0x3f) == 0x00)//如果能走的话..
                                {
                                    x = uavs_tmp[i].curr_xyz[0];
                                    y = uavs_tmp[i].curr_xyz[1];
                                    z = (uavs_tmp[i].curr_xyz[2] + 1 > map_tmp->fly_high_limit ? 
                                    (uavs_tmp[i].curr_xyz[2] - 1) : (uavs_tmp[i].curr_xyz[2] + 1));
                                    unsigned char level = (uavs_tmp[i].uav_para->value) / UAV_VAL_CVT_BASE;
                                    SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2], val_curr & 0x40);
                                    SetMapValue(map_tmp, x, y, z, val_next_tmp | (level & 0x3f));
                                    uavs_tmp[i].wait_time = 0;
                                    InsertTopNode(uavs_tmp[i].route, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2]);
                                }
                            }else//本来就是上下走的，说明他在堵你。。。没办法了，只能停住吧
                            {
                                x = uavs_tmp[i].curr_xyz[0];
                                y = uavs_tmp[i].curr_xyz[1];
                                z = uavs_tmp[i].curr_xyz[2];
                            }
                        }
                    }else//如果不是敌方，是己方
                    {
                        if (   uavs_tmp[i].route->next->cdnt_xyz[0] == map_tmp->parking_x
                            && uavs_tmp[i].route->next->cdnt_xyz[1] == map_tmp->parking_y
                            && uavs_tmp[i].route->next->cdnt_xyz[2] == 0) //如果下一步是回到老家了，就直接走
                        {
                            x = uavs_tmp[i].route->next->cdnt_xyz[0];
                            y = uavs_tmp[i].route->next->cdnt_xyz[1];
                            z = uavs_tmp[i].route->next->cdnt_xyz[2];
                            uavs_tmp[i].aim_xyz[0] = -1;
                            uavs_tmp[i].aim_xyz[1] = -1;
                            uavs_tmp[i].aim_xyz[2] = -1;  
                            uavs_tmp[i].wait_time = 0;
                            //删除路径的当前点！
                            DeleteTopNode(uavs_tmp[i].route);
                            unsigned char level = (uavs_tmp[i].uav_para->value) / UAV_VAL_CVT_BASE;
                            SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2], val_curr & 0x40);
                            SetMapValue(map_tmp, x, y, z, val_next | (level & 0x3f));
                            game_paras_tmp->back_charge_uavs_num--;
#ifdef PRINT_CHARGE_NUM
                            printf("uav%d cha->home\n", i);
#endif
                        }else
                        {//等待！
#ifdef PRINT_WAIT_CAUSE
                            printf("uav%d wait: our uav, wait_time%d, is_attack%d\n", i, uavs_tmp[i].wait_time, game_paras_tmp->is_attack_uav[i]);
#endif
                            x = uavs_tmp[i].curr_xyz[0];
                            y = uavs_tmp[i].curr_xyz[1];
                            z = uavs_tmp[i].curr_xyz[2];
                            uavs_tmp[i].wait_time++;if (uavs_tmp[i].wait_time > 30000)uavs_tmp[i].wait_time = 0;
                            if ((uavs_tmp[i].wait_time > UAV_MAX_WAIT_STEPS_NUM  //等待太长了,就往上或者下走
                                && uavs_tmp[i].curr_xyz[2] == uavs_tmp[i].route->next->cdnt_xyz[2])//而且本来不是上下走的
                                || (uavs_tmp[i].wait_time > UAV_MAX_WAIT_STEPS_NUM + 3))//或者如果实在等得太长了,也往上下走
                            {
                                unsigned char val_next_tmp = GetPointVal(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], 
                                                (uavs_tmp[i].curr_xyz[2] + 1 > map_tmp->fly_high_limit ? 
                                                (uavs_tmp[i].curr_xyz[2] - 1) : (uavs_tmp[i].curr_xyz[2] + 1)));
                                if ((val_next_tmp & 0x3f) == 0x00 //如果能走的话..如果不能走的话，还是会一直等待的！！
                                && !(game_paras_tmp->is_attack_uav[i] == 1 
                                    && uavs_tmp[i].curr_xyz[0] == map_tmp->enemy_parking_x 
                                    && uavs_tmp[i].curr_xyz[1] == map_tmp->enemy_parking_y
                                    && uavs_tmp[i].curr_xyz[2] <  map_tmp->fly_low_limit))//且不是已经到敌方老家的攻击uav
                                {
                                    x = uavs_tmp[i].curr_xyz[0];
                                    y = uavs_tmp[i].curr_xyz[1];
                                    z = (uavs_tmp[i].curr_xyz[2] + 1 > map_tmp->fly_high_limit ? 
                                    (uavs_tmp[i].curr_xyz[2] - 1) : (uavs_tmp[i].curr_xyz[2] + 1));
                                    unsigned char level = (uavs_tmp[i].uav_para->value) / UAV_VAL_CVT_BASE;
                                    SetMapValue(map_tmp, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2], val_curr & 0x40);
                                    SetMapValue(map_tmp, x, y, z, val_next_tmp | (level & 0x3f));
                                    uavs_tmp[i].wait_time = 0;
                                    InsertTopNode(uavs_tmp[i].route, uavs_tmp[i].curr_xyz[0], uavs_tmp[i].curr_xyz[1], uavs_tmp[i].curr_xyz[2]);
                                }
                            }                                 
                        }
                    }
                }
            }
            json_object* json_obj_tmp = json_object_new_object(); 
            json_object_object_add(json_obj_tmp, "no", json_object_new_int(i));
            json_object_object_add(json_obj_tmp, "x", json_object_new_int(x));
            json_object_object_add(json_obj_tmp, "y", json_object_new_int(y));
            json_object_object_add(json_obj_tmp, "z", json_object_new_int(z));
            json_object_object_add(json_obj_tmp, "goods_no", json_object_new_int(uavs_tmp[i].good_id));
            /**************************************电量修改！****************************************/
            /*******在家里，无论是刚回到家准备充电，还是一直在家******/
            if ( x == map_tmp->parking_x && y == map_tmp->parking_y && z == 0)
            {
                int ele_tmp = uavs_tmp[i].remain_electricity + uavs_tmp[i].uav_para->charge;
                uavs_tmp[i].remain_electricity = ( ele_tmp > uavs_tmp[i].uav_para->capacity ? uavs_tmp[i].uav_para->capacity : ele_tmp);
            }else
            /******携带了货物，不管是刚拿到还是已经拿到了*******/
            if (uavs_tmp[i].good_id != -1)
            {
                // printf("id:%d\n", uavs_tmp[i].good_id);
                // printf(":%d\n", goods_tmp[uavs_tmp[i].good_id].weight);
                int ele_tmp = uavs_tmp[i].remain_electricity - goods_tmp[uavs_tmp[i].good_id].weight;
                uavs_tmp[i].remain_electricity = ( ele_tmp <= 0 ? 0 : ele_tmp);
            }//其他情况下，电量保持不变

            json_object_object_add(json_obj_tmp, "remain_electricity", json_object_new_int(uavs_tmp[i].remain_electricity));

            json_object_array_add(json_obj_array, json_obj_tmp);  
        }
    }
    json_object_object_add(new_json_obj, "UAV_info", json_obj_array);
    // printf("val%d\n", game_paras_tmp->we_value);
    json_object *json_obj_array_purchase = json_object_new_array();  
    // printf("goods_real_num%d\n", game_paras_tmp->goods_real_num);
    //如果能买到就尝试再买，直到不能买
    //战场上有good，而且uav没有超过数量限制！
    for (int i = 0; (game_paras_tmp->goods_real_num > 0)
                    && (game_paras_tmp->uavs_real_num < UAV_MAX) 
                    && (i < GOOD_MAX_NUM) 
                    && (game_paras_tmp->we_value > 0) ; ++i)
    {
        int id_tmp = game_paras_tmp->goods_val_sort[i];
        if (!IsGoodFree(goods_tmp[id_tmp]))
        {
            int purchase_uav_id_tmp = game_paras_tmp->uavs_type_num - 1;
            int purchase_tmp = game_paras_tmp->uavs_val_sort[purchase_uav_id_tmp];
            while(purchase_uav_id_tmp >= 0  
                    && (UAVs_para[purchase_tmp].load_weight < goods_tmp[id_tmp].weight 
                            || UAVs_para[purchase_tmp].value > game_paras_tmp->we_value))
            {
                purchase_uav_id_tmp--;
                if (purchase_uav_id_tmp >= 0)
                    purchase_tmp = game_paras_tmp->uavs_val_sort[purchase_uav_id_tmp];
            }
            if (purchase_uav_id_tmp != -1)//说明找到合适的uav
            {
                int tmp = game_paras_tmp->uavs_val_sort[purchase_uav_id_tmp];
                game_paras_tmp->we_value -= UAVs_para[tmp].value;
                json_object* json_obj_purchase_tmp = json_object_new_object();
                json_object_object_add(json_obj_purchase_tmp, "purchase"
                            , json_object_new_string(UAVs_para[tmp].uav_type));
                json_object_array_add(json_obj_array_purchase, json_obj_purchase_tmp); 
                game_paras_tmp->uavs_real_num++; 
            }
        }
    }

        // while(UAVs_para[(id_tmp = UAVs_para_id[rand() % game_paras_tmp->uavs_type_num])].value < game_paras_tmp->we_value 
        //     && game_paras_tmp->uavs_real_num < UAV_MAX)
        // {
        //     game_paras_tmp->we_value -= UAVs_para[id_tmp].value;
        //     json_object* json_obj_purchase_tmp = json_object_new_object();
        //     json_object_object_add(json_obj_purchase_tmp, "purchase", json_object_new_string(UAVs_para[id_tmp].uav_type));
        //     json_object_array_add(json_obj_array_purchase, json_obj_purchase_tmp); 
        //     game_paras_tmp->uavs_real_num++; 
        // }
    json_object_object_add(new_json_obj, "purchase_UAV", json_obj_array_purchase);

    // 真正的上报程序！
    int length = strlen(json_object_to_json_string(new_json_obj));
    char* str = (char*)malloc(length + 8 + 1);
    char str_tmp[10] = {0};
    int posi = 0;
    MyItoa(length, str_tmp, 10);
    for (int i = 0; i < 8 - strlen(str_tmp); ++i)
        posi += sprintf(str + posi, "%s", "0");
    posi += sprintf(str + posi, "%s", str_tmp);
    sprintf(str + posi, "%s", json_object_to_json_string(new_json_obj));

    ClientSend(client_tmp, str, strlen(str));
    //不知道需不需要释放？还是释放array的时候是不是连带挂载的都释放了？而且释放new_obj的时候连带array都释放了？
    // for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
    //     json_object_put(json_obj_tmp[i]);
    // json_object_put(json_obj_tmp);

    // json_object_put(json_obj_array);
    json_object_put(new_json_obj);
    return 0;
}

/*************************************************
@Description:  开始时刻为0时要上报初始位置
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
int ReportInitInfos(struct _game_para* game_paras, struct _client* client, struct _uav* uavs)
{
    struct _game_para* game_paras_tmp = game_paras;
    struct _client* client_tmp = client;
    struct _uav* uavs_tmp = uavs;

    json_object* new_json_obj = json_object_new_object(); 
    json_object_object_add(new_json_obj, "token", json_object_new_string(game_paras_tmp->token));
    json_object_object_add(new_json_obj, "action", json_object_new_string("flyPlane"));
    json_object *json_obj_array = json_object_new_array();  
    // json_object** json_obj_tmp = (json_object**)malloc(sizeof(json_object*) * game_paras_tmp->uavs_num);

    for (int i = 0; i < game_paras_tmp->uavs_num; ++i) 
    {
        if (uavs_tmp[i].status != UAV_STAT_DESTROYED)//每一个有效uav
        {
            json_object* json_obj_tmp = json_object_new_object(); 
            json_object_object_add(json_obj_tmp, "no", json_object_new_int(i)); 
            json_object_object_add(json_obj_tmp, "x", json_object_new_int(uavs_tmp[i].curr_xyz[0]));
            json_object_object_add(json_obj_tmp, "y", json_object_new_int(uavs_tmp[i].curr_xyz[1]));
            json_object_object_add(json_obj_tmp, "z", json_object_new_int(uavs_tmp[i].curr_xyz[2]));
            json_object_object_add(json_obj_tmp, "goods_no", json_object_new_int(uavs_tmp[i].good_id));
            json_object_object_add(json_obj_tmp, "remain_electricity", json_object_new_int(uavs_tmp[i].remain_electricity));
            json_object_array_add(json_obj_array, json_obj_tmp);        
        }
    }

    json_object_object_add(new_json_obj, "UAV_info", json_obj_array);

    int length = strlen(json_object_to_json_string(new_json_obj));
    char* str = (char*)malloc(length + 8);
    char str_tmp[10] = {0};
    int posi = 0;
    MyItoa(length, str_tmp, 10);
    for (int i = 0; i < 8 - strlen(str_tmp); ++i)
        posi += sprintf(str + posi, "%s", "0");
    posi += sprintf(str + posi, "%s", str_tmp);
    sprintf(str + posi, "%s", json_object_to_json_string(new_json_obj));

    ClientSend(client_tmp, str, strlen(str));
    free(str);
    //不知道需不需要释放？还是释放array的时候是不是连带挂载的都释放了？而且释放new_obj的时候连带array都释放了？
    // for (int i = 0; i < game_paras_tmp->uavs_num; ++i)
    //     json_object_put(json_obj_tmp[i]);
    // json_object_put(json_obj_tmp);

    // json_object_put(json_obj_array);
    json_object_put(new_json_obj);
    return 0;
}
/*************************************************
@Description:  atoi
@Input: 
@Output: 
@Return: 0-成功   1-失败  
@Others: 
*************************************************/
char* MyItoa(int value, char *string, int radix)  
{  
    char zm[37]="0123456789abcdefghijklmnopqrstuvwxyz";  
    char aa[100]={0};  
  
    int sum=value;  
    char *cp=string;  
    int i=0;  
      
    if(radix<2||radix>36)//增加了对错误的检测  
    {  
        printf("error data!\n");  
        return string;  
    }  
  
    if(value<0)  
    {  
        printf("error data!\n"); 
        return string;  
    }  
  
    while(sum>0)  
    {  
        aa[i++]=zm[sum%radix];  
        sum/=radix;  
    }  
  
    for(int j=i-1;j>=0;j--)  
    {  
        *cp++ = aa[j];  
    }  
    *cp='\0';  
    return string;  
}  
/** 
  * 计算两个时间的间隔，得到时间差 
  * @param struct timeval* resule 返回计算出来的时间 
  * @param struct timeval* x 需要计算的前一个时间 
  * @param struct timeval* y 需要计算的后一个时间 
  * return -1 failure ,0 success 
**/  
unsigned long timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)  
{  
    // int nsec;  
  
    // if ( x->tv_sec>y->tv_sec )  
    //           return -1;  
  
    // if ( (x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec) )  
    //           return -1;  
  
    // result->tv_sec = ( y->tv_sec-x->tv_sec );  
    // result->tv_usec = ( y->tv_usec-x->tv_usec );  
  
    // if (result->tv_usec<0)  
    // {  
    //           result->tv_sec--;  
    //           result->tv_usec+=1000000;  
    // }  
    // int64_t ts_x = (int64_t)x->tv_sec*1000 + x->tv_usec/1000;
    // int64_t ts_y = (int64_t)y->tv_sec*1000 + y->tv_usec/1000;
    unsigned long diff = 1000000 * (y->tv_sec-x->tv_sec)+ y->tv_usec-x->tv_usec;

    return diff;
  
} 

int CountAttackUAVNums(struct _game_para* game_paras, struct _uav* uavs)
{
    int sum = 0;
    for (int i = 0; i < game_paras->uavs_num; ++i)
        if (uavs[i].status != UAV_STAT_DESTROYED)
            sum += game_paras->is_attack_uav[i];
    return sum;
}
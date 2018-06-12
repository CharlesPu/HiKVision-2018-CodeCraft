/*******************************
@@Author     : Charles
@@Date       : 2018-05-07
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/

#ifndef __TASKS_H__
#define __TASKS_H__ 

#include <sys/time.h> 
#include <time.h>  

#include "client.h"
#include "uavs.h"
#include "map.h"
#include "config.h"


int TaskInit(struct _game_para*, struct _uav* uavs, struct _good*, struct _map*);
int PreProcess(struct _game_para*, struct _client* client, struct _map* map, struct _uav* uavs, struct _good*);
int DistributeTargets(struct _game_para*, struct _map* map, struct _uav* uavs, struct _good*);
int FlyUAVs(struct _game_para*, struct _client* client, struct _map* map, struct _uav* uavs, struct _good*);

int ReportInitInfos(struct _game_para*, struct _client* client, struct _uav* uavs);
char* MyItoa(int value,char *string,int radix);
unsigned long timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y); 
int CountAttackUAVNums(struct _game_para* game_paras, struct _uav* uavs);

#endif
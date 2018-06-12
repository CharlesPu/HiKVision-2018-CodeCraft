/*******************************
@@Author     : Charles
@@Date       : 2018-05-07
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <netinet/in.h>
#include <sys/epoll.h>
#include "config.h"

struct _client
{
	int clie_sock;
	struct sockaddr_in serv_addr; //所连接的服务器的信息
	char c_tx_buf[BUF_LENGTH]; //发送缓冲
	char c_rx_buf[BUF_LENGTH]; //接收缓冲
};

int ClientInit(struct _client* client, char* ip, int port); 
int ClientSend(struct _client* client, const char* data, int size);
int ClientRecv(struct _client* client);




#endif
/*******************************
@@Author     : Charles
@@Date       : 2018-05-07
@@Mail       : pu17rui@sina.com
@@Description:             
*******************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>//close、read、write函数需要
// #include <errno.h>
#include <fcntl.h>//F_GETFL设置非阻塞时需要
#include <sys/socket.h>
#include <sys/types.h>//数据类型定义
#include <arpa/inet.h>//ip地址转换

#include "client.h"

/*************************************************
@Description: 初始化client连接，改变clien结构体中变量
@Input: struct _client结构体地址
@Output: 
@Return: 0-成功   1-失败 
@Others: 
*************************************************/
int ClientInit(struct _client* client, char* ip, int port)
{
	struct _client* clie_tmp = client;

	if ((clie_tmp->clie_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to obtain Socket Descriptor!\r\n");
        return 1;
    }

    bzero(&(clie_tmp->serv_addr), sizeof(clie_tmp->serv_addr));
    clie_tmp->serv_addr.sin_family = AF_INET;
    clie_tmp->serv_addr.sin_port = htons(port);
    //_addrLocal.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, ip, &(clie_tmp->serv_addr.sin_addr.s_addr)); 
	
	int recvbuf=3000;		 //recv的内核缓冲buf为6000字节
	// int len = sizeof(recvbuf); 
	setsockopt(clie_tmp->clie_sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf)); 
    // getsockopt(clie_tmp->clie_sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t*)&len);  
    // printf("the receive buffer size after setting is %d\n", recvbuf);

    int nRet = 0;
    struct timeval timeout1  = {6, 0};           ///< send recv 数据超时设置为6秒
    nRet = setsockopt(clie_tmp->clie_sock,SOL_SOCKET, SO_SNDTIMEO,(const char*)&timeout1,sizeof(timeout1));
    if (nRet != 0)
    {
    	perror("Failed to set send timeout!\r\n");
        return 1;
    }
    struct timeval timeout2  = {8, 0};           ///< send recv 数据超时设置为6秒
    nRet = setsockopt(clie_tmp->clie_sock,SOL_SOCKET, SO_RCVTIMEO,(const char*)&timeout2,sizeof(timeout2));
    if (nRet != 0)
    {
    	perror("Failed to set recv timeout!\r\n");
        return 1;
    }

    //阻塞在这里直到连接成功
    while(connect(clie_tmp->clie_sock, (struct sockaddr *)&(clie_tmp->serv_addr), sizeof(clie_tmp->serv_addr)));
    // printf("OK: Client[%d] has connected to %s : %d\n", clie_tmp->clie_sock, inet_ntoa(clie_tmp->serv_addr.sin_addr), port);

    //设置非阻塞
    // int opts = fcntl(clie_tmp->clie_sock, F_GETFL);
    // if(opts < 0)
    // {
    //     perror("clie fcntl fail!\r\n");
    //     return 1;
    // }
    // opts |= O_NONBLOCK;
    // if(fcntl(clie_tmp->clie_sock, F_SETFL, opts) < 0)
    // {
    //     perror("clie fcntl fail!\r\n");
    //     return 1;
    // }

    memset(clie_tmp->c_tx_buf, 0, BUF_LENGTH);
	memset(clie_tmp->c_rx_buf, 0, BUF_LENGTH);

    return 0;
}

/*************************************************
@Description: client发送函数
@Input: struct _client结构体地址，发送的数据，发送的数据长度
@Output: 
@Return: 发送成功的缓冲区长度
@Others: 
*************************************************/
int ClientSend(struct _client* client, const char* data, int size)
{
	int ret=0;
	struct _client* clie_tmp = client;
	ret = send(clie_tmp->clie_sock, data, size, 0); 
#ifdef PRINT_SEND
	if(ret == size)
		{
		/**************最终发出去的帧，调试用 ***************/	
			// printf("client_send_data:\r\n");
			// for(int i = 0; i < ret ; i++)
			//    printf("0x%02x ", data[i]);
			printf("%s", data);
			printf("\n");
		}
	else
		printf("send out by client fail!\r\n");	
#endif
#ifdef PRINT_COST_TIME
    printf("has sent out by client ......................................................................................^\r\n");
#endif
	return ret;	
}

/*************************************************
@Description: client接收函数
@Input: struct _client结构体地址
@Output: 
@Return: 0-成功   -1-失败   其他-读取到的长度
@Others: 
*************************************************/
int ClientRecv(struct _client* client)
{
	struct _client* clie_tmp = client;
	memset(clie_tmp->c_rx_buf, 0, BUF_LENGTH);
	int ret = read(clie_tmp->clie_sock, clie_tmp->c_rx_buf, BUF_LENGTH);
	if(ret < 0)
	{
		// printf("client read erro!\r\n");
		return -1;	
	}else 
	if(ret > 0)
	{
#ifdef PRINT_RECV
	/**************最初接收到的帧，调试用 ***************/
		// printf("data length: %d ", ret);
		// printf("client_rec_data:\r\n");//将从服务器读到的数据，在屏幕上输出
		// for (int i = 0; i < ret; i++)
		// 	printf("0x%02x ", clie_tmp->c_rx_buf[i]);
		printf("%s", clie_tmp->c_rx_buf);
		printf("\n");
#endif
	}
	return ret;
}
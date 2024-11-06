#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <arpa/inet.h>

#include "eventloop.h"
#include "thread_pool.h"
#include "log.h"

typedef struct listener
{
    int lfd;
    unsigned short port;
} listener_t;

typedef struct tcp_server
{
    int thread_num;
    eventloop_t *main_loop;
    thread_pool_t *thread_pool;
    listener_t *listener;
} tcp_server_t;

// 初始化
tcp_server_t *tcp_server_init(unsigned short port, int thread_num);
// 初始化监听
listener_t *listener_init(unsigned short port);
// 启动服务器
void tcp_server_run(tcp_server_t *server);

#endif
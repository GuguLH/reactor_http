#include "tcp_server.h"
#include "tcp_connection.h"

int accept_connection(void *arg)
{
    tcp_server_t *server = (tcp_server_t *)arg;
    // 和客户端建立连接
    int cfd = accept(server->listener->lfd, NULL, NULL);
    // 从线程池取出一个子线程的反应堆模型,处理客户端连接请求
    eventloop_t *ev_loop = take_worker_eventloop(server->thread_pool);
    // 将cfd放到tcp_connection中处理
    tcp_connection_t *conn = tcp_connection_init(cfd, ev_loop);
}

tcp_server_t *tcp_server_init(unsigned short port, int thread_num)
{
    tcp_server_t *tcp = (tcp_server_t *)malloc(sizeof(tcp_server_t));
    tcp->listener = listener_init(port);
    tcp->main_loop = eventloop_init();
    tcp->thread_num = thread_num;
    tcp->thread_pool = thread_pool_init(tcp->main_loop, thread_num);
    return tcp;
}

listener_t *listener_init(unsigned short port)
{
    listener_t *listener = (listener_t *)malloc(sizeof(listener_t));
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("Error socket()");
        return NULL;
    }
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("Error setsockopt()");
        return NULL;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("Error bind()");
        return NULL;
    }
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("Error listen()");
        return NULL;
    }

    listener->lfd = lfd;
    listener->port = port;

    return listener;
}

void tcp_server_run(tcp_server_t *server)
{
    // 启动线程池
    thread_pool_run(server->thread_pool);
    // 添加检测任务
    // 初始化一个channel
    channel_t *channel = channel_init(server->listener->lfd, READ_EVENT, accept_connection, NULL, NULL, server);
    eventloop_add_task(server->main_loop, channel, ADD);
    // 启动反应堆模型
    Debug("服务器程序已经启动了...");
    eventloop_run(server->main_loop);
}
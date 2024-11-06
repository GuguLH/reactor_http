#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>

#include "dispatcher.h"
#include "channel_map.h"

typedef struct dispatcher dispatcher_t;
extern dispatcher_t epoll_dispatcher;
extern dispatcher_t poll_dispatcher;
extern dispatcher_t select_dispatcher;

enum ELE_TYPE
{
    ADD,
    DELETE,
    MODIFY
};

// 定义任务队列的节点
typedef struct channel_ele
{
    int type;
    channel_t *channel;
    struct channel_ele *next;
} channel_ele_t;

typedef struct eventloop
{
    bool is_quit;
    dispatcher_t *dispatcher;
    void *dispatcher_data;
    // 任务队列
    channel_ele_t *head;
    channel_ele_t *tail;
    // map
    channel_map_t *channel_map;
    // 线程ID,name,mutex
    pthread_t thread_id;
    char thread_name[32];
    pthread_mutex_t mutex;
    int socket_pair[2]; // 存储用于本地通信的fds
} eventloop_t;

// 初始化
eventloop_t *eventloop_init();
eventloop_t *eventloop_init_wp(const char *thread_name);
// 启动
int eventloop_run(eventloop_t *ev_loop);
// 处理激活的文件fd
int event_activate(eventloop_t *ev_loop, int fd, int event);
// 往任务队列添加任务
int eventloop_add_task(eventloop_t *ev_loop, channel_t *channel, int type);
// 处理任务队列中的任务
int eventloop_process_task(eventloop_t *ev_loop);
// 处理dispatcher中的节点
int eventloop_add(eventloop_t *ev_loop, channel_t *channel);
int eventloop_remove(eventloop_t *ev_loop, channel_t *channel);
int eventloop_modify(eventloop_t *ev_loop, channel_t *channel);
// 释放channel
int destory_channel(eventloop_t *ev_loop, channel_t *channel);

#endif
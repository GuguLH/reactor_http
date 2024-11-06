#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <unistd.h>

#include "channel.h"
#include "eventloop.h"

typedef struct eventloop eventloop_t;

typedef struct dispatcher
{
    // 初始化
    void *(*init)();
    // 添加
    int (*add)(channel_t *channel, eventloop_t *ev_loop);
    // 删除
    int (*remove)(channel_t *channel, eventloop_t *ev_loop);
    // 修改
    int (*modify)(channel_t *channel, eventloop_t *ev_loop);
    // 事件检测
    int (*dispatch)(eventloop_t *ev_loop, int timeout);
    // 清除数据
    int (*clear)(eventloop_t *ev_loop);
} dispatcher_t;

#endif
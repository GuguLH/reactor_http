#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef int (*handle_func)(void *arg);

enum FD_EVENT
{
    READ_EVENT = 0x02,
    WRITE_EVENT = 0x04
};

typedef struct channel
{
    // 文件描述符
    int fd;
    // 事件
    int events;
    // 回调函数
    handle_func read_callback;
    handle_func write_callback;
    handle_func destory_callback;
    // 回调函数的参数
    void *arg;
} channel_t;

// 初始化
channel_t *channel_init(int fd, int events, handle_func read_func, handle_func write_func, handle_func destory_func, void *arg);
// 修改fd的写事件
void write_event_enable(channel_t *channel, bool flag);
// 判断是否要检测写事件
bool is_write_event_enable(channel_t *channel);

#endif
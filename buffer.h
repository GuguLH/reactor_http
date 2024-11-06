#ifndef __BUFFER_H__
#define __BUFFER_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

typedef struct buffer
{
    char *data;
    int capacity;
    int read_pos;
    int write_pos;
} buffer_t;

// 初始化
buffer_t *buffer_init(int size);
// 销毁
void buffer_destory(buffer_t *buf);
// 扩容
void buffer_resize(buffer_t *buf, int size);
// 得到剩余可写的内存容量
int buffer_writeable_size(buffer_t *buf);
// 得到剩余可读的内存容量
int buffer_readable_size(buffer_t *buf);
// 写内存
int buffer_append_data(buffer_t *buf, const char *data, int size);
int buffer_append_string(buffer_t *buf, const char *data);
int buffer_socket_read(buffer_t *buf, int fd);
// 根据\r\n取出一行,找到其在数据块中的位置,返回该位置
char *buffer_find_crlf(buffer_t *buf);
// 发送数据
int buffer_send_data(buffer_t *buf, int socket);

#endif
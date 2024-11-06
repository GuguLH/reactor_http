#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

// #define MSG_SEND_AUTO

#include "eventloop.h"
#include "buffer.h"
#include "channel.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"

typedef struct tcp_connection
{
    eventloop_t *ev_loop;
    channel_t *channel;
    buffer_t *read_buf;
    buffer_t *write_buf;
    char name[32];
    // http4.jpg
    http_request_t *request;
    http_response_t *response;
} tcp_connection_t;

// 初始化
tcp_connection_t *tcp_connection_init(int fd, eventloop_t *ev_loop);
int tcp_connection_destory(void *arg);

#endif
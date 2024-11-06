#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <strings.h>

#include "buffer.h"

#define RESP_HEADER_SZ 16

enum HTTP_RESPONSE_CODE
{
    UNKNOWN,
    OK = 200,
    MOVED_PERMANENTLY = 301,
    MOVED_TEMPORARILY = 302,
    BAD_REQUEST = 400,
    NOT_FOUND = 404
};

typedef struct response_header
{
    char key[32];
    char value[128];
} response_header_t;

// 定义一个函数指针,用于组织要返回给客户端的数据块
typedef void (*response_body)(const char *file_name, buffer_t *send_buf, int socket);

typedef struct http_response
{
    // 状态行
    enum HTTP_RESPONSE_CODE status_code;
    char status_msg[128];
    char file_name[128];
    // 响应头 - 键值对
    response_header_t *headers;
    int header_num;
    response_body send_data_func;
} http_response_t;

// 初始化
http_response_t *http_response_init();
// 销毁
void http_response_destory(http_response_t *resp);
// 添加响应头
void http_response_add_header(http_response_t *resp, const char *key, const char *value);
// 组织http响应数据
void http_response_prepare_msg(http_response_t *resp, buffer_t *send_buf, int socket);

#endif
#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#include "buffer.h"
#include "http_response.h"

#define HEADER_SZ 12

enum HTTP_REQUEST_STATE
{
    REQ_LINE,
    REQ_HEADERS,
    REQ_BODY,
    REQ_DONE
};

typedef struct request_header
{
    char *key;
    char *value;
} request_header_t;

typedef struct http_request
{
    char *method;
    char *url;
    char *version;
    request_header_t *req_headers;
    int req_header_num;
    enum HTTP_REQUEST_STATE cur_state;
} http_request_t;

// 初始化
http_request_t *http_request_init();
// 重置
void http_request_reset(http_request_t *req);
void http_request_reset_ex(http_request_t *req);
void http_request_destory(http_request_t *req);
// 获取当前处理状态
enum HTTP_REQUEST_STATE http_request_state(http_request_t *req);
// 添加请求头
void http_request_add_header(http_request_t *req, const char *key, const char *value);
// 根据key得到value
char *http_request_get_header(http_request_t *req, const char *key);
// 解析请求行
bool parse_http_request_line(http_request_t *req, buffer_t *read_buf);
// 解析请求头
bool parse_http_request_header(http_request_t *req, buffer_t *read_buf);
// 解析http请求协议
bool parse_http_request(http_request_t *req, buffer_t *read_buf, http_response_t *resp, buffer_t *write_buf, int sockfd);
// 处理http请求协议
bool process_http_request(http_request_t *req, http_response_t *resp);

#endif
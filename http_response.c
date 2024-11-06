#include "http_response.h"

http_response_t *http_response_init()
{
    http_response_t *resp = (http_response_t *)malloc(sizeof(http_response_t));
    resp->header_num = 0;
    resp->headers = (response_header_t *)malloc(sizeof(response_header_t) * RESP_HEADER_SZ);
    resp->status_code = UNKNOWN;
    // 初始化数组
    bzero(resp->headers, sizeof(response_header_t) * RESP_HEADER_SZ);
    bzero(resp->status_msg, sizeof(resp->status_msg));
    bzero(resp->file_name, sizeof(resp->file_name));
    resp->send_data_func = NULL;
    return resp;
}

void http_response_destory(http_response_t *resp)
{
    if (resp != NULL)
    {
        free(resp->headers);
        free(resp);
    }
}

void http_response_add_header(http_response_t *resp, const char *key, const char *value)
{
    if (resp == NULL || key == NULL || value == NULL)
    {
        return;
    }
    strcpy(resp->headers[resp->header_num].key, key);
    strcpy(resp->headers[resp->header_num].value, value);
    resp->header_num++;
}

void http_response_prepare_msg(http_response_t *resp, buffer_t *send_buf, int socket)
{
    // 状态行
    char tmp[1024] = {0};
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", resp->status_code, resp->status_msg);
    buffer_append_string(send_buf, tmp);
    // 响应头
    for (int i = 0; i < resp->header_num; i++)
    {
        sprintf(tmp, "%s: %s\r\n", resp->headers[i].key, resp->headers[i].value);
        buffer_append_string(send_buf, tmp);
    }
    // 空行
    buffer_append_string(send_buf, "\r\n");

#ifndef MSG_SEND_AUTO
    buffer_send_data(send_buf, socket);
#endif

    // 回复的数据
    resp->send_data_func(resp->file_name, send_buf, socket);
}
#include "http_request.h"

http_request_t *http_request_init()
{
    http_request_t *request = (http_request_t *)malloc(sizeof(http_request_t));
    http_request_reset(request);
    request->req_headers = (request_header_t *)malloc(sizeof(request_header_t) * HEADER_SZ);
    return request;
}

void http_request_reset(http_request_t *req)
{
    req->cur_state = REQ_LINE;
    req->method = NULL;
    req->url = NULL;
    req->version = NULL;
    req->req_header_num = 0;
}

void http_request_reset_ex(http_request_t *req)
{
    free(req->url);
    free(req->method);
    free(req->version);
    if (req->req_headers != NULL)
    {
        for (int i = 0; i < req->req_header_num; i++)
        {
            free(req->req_headers[i].key);
            free(req->req_headers[i].value);
        }
        free(req->req_headers);
    }
    http_request_reset(req);
}

void http_request_destory(http_request_t *req)
{
    if (req != NULL)
    {
        http_request_reset_ex(req);
        free(req);
    }
}

enum HTTP_REQUEST_STATE http_request_state(http_request_t *req)
{
    return req->cur_state;
}

void http_request_add_header(http_request_t *req, const char *key, const char *value)
{
    req->req_headers[req->req_header_num].key = (char *)key;
    req->req_headers[req->req_header_num].value = (char *)value;
    req->req_header_num++;
}

char *http_request_get_header(http_request_t *req, const char *key)
{
    if (req != NULL)
    {
        for (int i = 0; i < req->req_header_num; i++)
        {
            if (strncasecmp(req->req_headers[i].key, key, strlen(key)) == 0)
            {
                return req->req_headers[i].value;
            }
        }
    }
    return NULL;
}

char *split_request_line(const char *start, const char *end, const char *sub, char **ptr)
{
    char *space = (char *)end;
    if (sub != NULL)
    {
        space = memmem(start, end - start, sub, strlen(sub));
        assert(space != NULL);
    }

    int length = space - start;
    char *tmp = (char *)malloc(length + 1);
    strncpy(tmp, start, length);
    tmp[length] = '\0';
    *ptr = tmp;
    return space + 1;
}

bool parse_http_request_line(http_request_t *req, buffer_t *read_buf)
{
    // 读出请求行,保存字符串结束地址
    char *end = buffer_find_crlf(read_buf);
    // 保存字符串起始地址
    char *start = read_buf->data + read_buf->read_pos;
    // 请求行总长度
    int line_size = end - start;

    if (line_size)
    {
        // 请求的方式
        start = split_request_line(start, end, " ", &req->method);
        start = split_request_line(start, end, " ", &req->url);
        split_request_line(start, end, NULL, &req->version);

        // 为解析请求头做准备
        read_buf->read_pos += line_size;
        read_buf->read_pos += 2;
        // 修改状态
        req->cur_state = REQ_HEADERS;
        return true;
    }

    return false;
}

// 目前只处理请求头中的一行
bool parse_http_request_header(http_request_t *req, buffer_t *read_buf)
{
    char *end = buffer_find_crlf(read_buf);
    if (end != NULL)
    {
        char *start = read_buf->data + read_buf->read_pos;
        int line_size = end - start;
        // 基于: 搜索字符
        char *middle = memmem(start, line_size, ": ", 2);
        if (middle != NULL)
        {
            char *key = malloc(middle - start + 1);
            strncpy(key, start, middle - start);
            key[middle - start] = '\0';

            char *value = malloc(end - middle - 2 + 1);
            strncpy(value, middle + 2, end - middle - 2);
            value[end - middle - 2] = '\0';

            http_request_add_header(req, key, value);
            // 移动数据位置
            read_buf->read_pos += line_size;
            read_buf->read_pos += 2;
        }
        else
        {
            // 请求头解析完毕,跳过空行
            read_buf->read_pos += 2;
            // 修改解析状态
            req->cur_state = REQ_DONE;
        }
        return true;
    }
    return false;
}

bool parse_http_request(http_request_t *req, buffer_t *read_buf, http_response_t *resp, buffer_t *write_buf, int sockfd)
{
    bool flag = true;
    while (req->cur_state != REQ_DONE)
    {
        switch (req->cur_state)
        {
        case REQ_LINE:
            flag = parse_http_request_line(req, read_buf);
            break;
        case REQ_HEADERS:
            flag = parse_http_request_header(req, read_buf);
            break;
        case REQ_BODY:
            break;
        default:
            break;
        }
        if (!flag)
        {
            return flag;
        }
        if (req->cur_state == REQ_DONE)
        {
            // 服务器回复数据
            // 1 根据解析出的原始数据,对客户端的请求作出处理
            process_http_request(req, resp);
            // 2 组织响应数据并发送给客户端
            http_response_prepare_msg(resp, write_buf, sockfd);
        }
    }
    req->cur_state = REQ_LINE;
    return flag;
}

int hex_to_dec(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return 0;
}

void decode_str(char *to, char *from)
{
    for (; *from != '\0'; to++, from++)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            *to = hex_to_dec(from[1]) * 16 + hex_to_dec(from[2]);
            from += 2;
        }
        else
        {
            *to = *from;
        }
    }
    *to = '\0';
}

const char *get_file_type(const char *name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char *dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8"; // 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void send_file(const char *file_name, buffer_t *send_buf, int cfd)
{
    int fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        return;
    }

#if 1
    while (1)
    {
        char buf[1024] = {0};
        int len = read(fd, buf, sizeof(buf));
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            buffer_append_data(send_buf, buf, len);
#ifndef MSG_SEND_AUTO
            buffer_send_data(send_buf, cfd);
#endif
        }
        else if (len == 0)
        {
            printf("文件发送完毕!\n");
            break;
        }
        else
        {
            printf("文件发送失败!\n");
            close(fd);
        }
    }
#else
    off_t offset = 0;
    // 尝试使用sendfile
    int f_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // 文件指针一定要移动回去啊！！！
    while (offset < f_size)
    {
        int ret = sendfile(cfd, fd, &offset, f_size - offset);
        // printf("Ret = %d\n", ret);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 如果资源不可用，继续等待并尝试重新发送
            usleep(1000); // 等待一会儿
            continue;
        }
        else
        {
            close(fd);
            close(cfd);
            return;
        }
    }

#endif
    close(fd);
}

void send_dir(const char *dir_name, buffer_t *send_buf, int cfd)
{
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir_name);

    struct dirent **namelist;
    int num = scandir(dir_name, &namelist, NULL, alphasort);
    for (int i = 0; i < num; i++)
    {
        char *name = namelist[i]->d_name;
        struct stat st;
        char sub_path[1024] = {0};
        sprintf(sub_path, "%s/%s", dir_name, name);
        stat(sub_path, &st);
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href="">name</a>
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        else
        {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }
        buffer_append_string(send_buf, buf);
#ifndef MSG_SEND_AUTO
        buffer_send_data(send_buf, cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    buffer_append_string(send_buf, buf);
#ifndef MSG_SEND_AUTO
    buffer_send_data(send_buf, cfd);
#endif
    free(namelist);
}

bool process_http_request(http_request_t *req, http_response_t *resp)
{
    // GET /abc.jpg
    if (strcasecmp(req->method, "GET") != 0)
    {
        return false;
    }
    // 处理特殊字符
    decode_str(req->url, req->url);
    char *file = NULL;
    if (strcmp(req->url, "/") == 0)
    {
        file = "./";
    }
    else
    {
        file = req->url + 1;
    }

    int ret = 0;
    struct stat st;
    ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在,返回404
        strcpy(resp->file_name, "404.html");
        resp->status_code = NOT_FOUND;
        strcpy(resp->status_msg, "Not Found");
        // 响应头
        http_response_add_header(resp, "Content-type", get_file_type(".html"));
        resp->send_data_func = send_file;
        return 0;
    }

    strcpy(resp->file_name, file);
    resp->status_code = OK;
    strcpy(resp->status_msg, "OK");
    if (S_ISDIR(st.st_mode))
    {
        // 把目录内容发送给客户端
        strcpy(resp->file_name, file);
        resp->status_code = OK;
        strcpy(resp->status_msg, "OK");
        // 响应头
        http_response_add_header(resp, "Content-type", get_file_type(".html"));
        resp->send_data_func = send_dir;
    }
    else
    {
        // 把文件发送给客户端
        // 响应头
        char tmp[12] = {0};
        sprintf(tmp, "%ld", st.st_size);
        http_response_add_header(resp, "Content-type", get_file_type(file));
        http_response_add_header(resp, "Content-length", tmp);
        resp->send_data_func = send_file;
    }
    return false;
}

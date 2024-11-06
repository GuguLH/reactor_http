#include "tcp_connection.h"

int process_read(void *arg)
{
    tcp_connection_t *conn = (tcp_connection_t *)arg;
    // 接收数据
    int count = buffer_socket_read(conn->read_buf, conn->channel->fd);
    Debug("接收到的http请求数据: %s", conn->read_buf->data + conn->read_buf->read_pos);
    if (count > 0)
    {
        // 接收到了http请求,解析http请求
        int socket = conn->channel->fd;
#ifdef MSG_SEND_AUTO
        write_event_enable(conn->channel, true);
        eventloop_add_task(conn->ev_loop, conn->channel, MODIFY);
#endif
        bool flag = parse_http_request(conn->request, conn->read_buf, conn->response, conn->write_buf, socket);
        if (!flag)
        {
            // 解析失败
            char *err_msg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            buffer_append_string(conn->write_buf, err_msg);
        }
    }
    else
    {
#ifdef MSG_SEND_AUTO
        // 断开连接
        eventloop_add_task(conn->ev_loop, conn->channel, DELETE);
#endif
    }
#ifndef MSG_SEND_AUTO
    // 断开连接
    eventloop_add_task(conn->ev_loop, conn->channel, DELETE);
#endif
    return 0;
}

int process_write(void *arg)
{
    // Debug("开始发送数据了(基于写事件发送)");
    tcp_connection_t *conn = (tcp_connection_t *)arg;
    // 发送数据
    int count = buffer_send_data(conn->write_buf, conn->channel->fd);
    if (count > 0)
    {
        // 判断数据是否被全部发送出去了
        if (buffer_readable_size(conn->write_buf) == 0)
        {
            // 1 不再监测写事件 - 修改channel中保存的事件
            write_event_enable(conn->channel, false);
            // 2 修改dispatcer监测的集合 - 添加任务节点
            eventloop_add_task(conn->ev_loop, conn->channel, MODIFY);
            // 3 删除这个节点
            eventloop_add_task(conn->ev_loop, conn->channel, DELETE);
        }
    }
}

tcp_connection_t *tcp_connection_init(int fd, eventloop_t *ev_loop)
{
    tcp_connection_t *conn = (tcp_connection_t *)malloc(sizeof(tcp_connection_t));
    conn->ev_loop = ev_loop;
    conn->read_buf = buffer_init(10240);
    conn->write_buf = buffer_init(10240);
    conn->request = http_request_init();
    conn->response = http_response_init();
    sprintf(conn->name, "Connection-%d", fd);
    conn->channel = channel_init(fd, READ_EVENT, process_read, process_write, tcp_connection_destory, conn);

    eventloop_add_task(ev_loop, conn->channel, ADD);
    Debug("和客户端建立连接, thread_name: %s, thtread_id: %ld, conn_name: %s", ev_loop->thread_name, ev_loop->thread_id, conn->name);
    return conn;
}

int tcp_connection_destory(void *arg)
{
    tcp_connection_t *conn = (tcp_connection_t *)arg;
    if (conn != NULL)
    {
        if (conn->read_buf && buffer_readable_size(conn->read_buf) == 0 &&
            conn->write_buf && buffer_writeable_size(conn->write_buf) == 0)
        {
            destory_channel(conn->ev_loop, conn->channel);
            buffer_destory(conn->read_buf);
            buffer_destory(conn->write_buf);
            http_request_destory(conn->request);
            http_response_destory(conn->response);
            free(conn);
        }
    }
    Debug("连接断开,释放资源,conn_name: %s", conn->name);
    return 0;
}

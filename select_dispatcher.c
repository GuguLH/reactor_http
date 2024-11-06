#include <sys/select.h>
#include "dispatcher.h"

#define FD_MAX 1024

typedef struct select_data
{
    fd_set read_set;
    fd_set write_set;
} select_data_t;

static void *select_init();
static int select_add(channel_t *channel, eventloop_t *ev_loop);
static int select_remove(channel_t *channel, eventloop_t *ev_loop);
static int select_modify(channel_t *channel, eventloop_t *ev_loop);
static int select_dispatch(eventloop_t *ev_loop, int timeout);
static int select_clear(eventloop_t *ev_loop);

dispatcher_t select_dispatcher =
    {
        select_init,
        select_add,
        select_remove,
        select_modify,
        select_dispatch,
        select_clear};

static void *select_init()
{
    select_data_t *data = (select_data_t *)malloc(sizeof(select_data_t));
    FD_ZERO(&data->read_set);
    FD_ZERO(&data->write_set);
    return data;
}

static void set_fd_set(channel_t *channel, select_data_t *data)
{
    if (channel->events & READ_EVENT)
    {
        FD_SET(channel->fd, &data->read_set);
    }
    if (channel->events & WRITE_EVENT)
    {
        FD_SET(channel->fd, &data->write_set);
    }
}

static void clear_fd_set(channel_t *channel, select_data_t *data)
{
    if (channel->events & READ_EVENT)
    {
        FD_CLR(channel->fd, &data->read_set);
    }
    if (channel->events & WRITE_EVENT)
    {
        FD_CLR(channel->fd, &data->write_set);
    }
}

static int select_add(channel_t *channel, eventloop_t *ev_loop)
{
    select_data_t *data = (select_data_t *)ev_loop->dispatcher_data;
    if (channel->fd >= FD_MAX)
    {
        return -1;
    }
    set_fd_set(channel, data);
    return 0;
}

static int select_remove(channel_t *channel, eventloop_t *ev_loop)
{
    select_data_t *data = (select_data_t *)ev_loop->dispatcher_data;
    clear_fd_set(channel, data);
    // 通过channel释放对应的TcpConnection资源
    channel->destory_callback(channel->arg);
    return 0;
}

static int select_modify(channel_t *channel, eventloop_t *ev_loop)
{
    select_data_t *data = (select_data_t *)ev_loop->dispatcher_data;
    set_fd_set(channel, data);
    clear_fd_set(channel, data);
    return 0;
}

static int select_dispatch(eventloop_t *ev_loop, int timeout)
{
    select_data_t *data = (select_data_t *)ev_loop->dispatcher_data;
    fd_set rd_tmp = data->read_set;
    fd_set wr_tmp = data->write_set;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    int count = select(FD_MAX, &rd_tmp, &wr_tmp, NULL, &tv);
    if (count == -1)
    {
        perror("Error select()");
        return -1;
    }
    for (int i = 0; i < FD_MAX; i++)
    {
        if (FD_ISSET(i, &rd_tmp))
        {
            // 处理读就绪
            event_activate(ev_loop, i, READ_EVENT);
        }
        if (FD_ISSET(i, &wr_tmp))
        {
            // 处理写就绪
            event_activate(ev_loop, i, WRITE_EVENT);
        }
    }
    return 0;
}

static int select_clear(eventloop_t *ev_loop)
{
    select_data_t *data = (select_data_t *)ev_loop->dispatcher_data;
    free(data);
    return 0;
}
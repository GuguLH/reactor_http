#include <poll.h>
#include "dispatcher.h"

#define FD_MAX 1024

typedef struct poll_data
{
    int max_fd;
    struct pollfd fds[FD_MAX];
} poll_data_t;

static void *poll_init();
static int poll_add(channel_t *channel, eventloop_t *ev_loop);
static int poll_remove(channel_t *channel, eventloop_t *ev_loop);
static int poll_modify(channel_t *channel, eventloop_t *ev_loop);
static int poll_dispatch(eventloop_t *ev_loop, int timeout);
static int poll_clear(eventloop_t *ev_loop);

dispatcher_t poll_dispatcher =
    {
        poll_init,
        poll_add,
        poll_remove,
        poll_modify,
        poll_dispatch,
        poll_clear};

static void *poll_init()
{
    poll_data_t *data = (poll_data_t *)malloc(sizeof(poll_data_t));
    data->max_fd = 0;
    for (int i = 0; i < FD_MAX; i++)
    {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
}

static int poll_add(channel_t *channel, eventloop_t *ev_loop)
{
    poll_data_t *data = (poll_data_t *)ev_loop->dispatcher_data;

    int events = 0;
    if (channel->events & READ_EVENT)
    {
        events |= POLLIN;
    }
    if (channel->events & WRITE_EVENT)
    {
        events |= POLLOUT;
    }

    int i = 0;
    for (i = 0; i < FD_MAX; i++)
    {
        if (data->fds[i].fd == -1)
        {
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->max_fd = i > data->max_fd ? i : data->max_fd;
            break;
        }
    }
    if (i >= FD_MAX)
    {
        return -1;
    }
    return 0;
}

static int poll_remove(channel_t *channel, eventloop_t *ev_loop)
{
    poll_data_t *data = (poll_data_t *)ev_loop->dispatcher_data;

    int i = 0;
    for (i = 0; i < FD_MAX; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].fd = -1;
            data->fds[i].events = 0;
            data->fds[i].revents = 0;
            break;
        }
    }
    // 通过channel释放对应的TcpConnection资源
    channel->destory_callback(channel->arg);
    if (i >= FD_MAX)
    {
        return -1;
    }
    return 0;
}

static int poll_modify(channel_t *channel, eventloop_t *ev_loop)
{
    poll_data_t *data = (poll_data_t *)ev_loop->dispatcher_data;

    int events = 0;
    if (channel->events & READ_EVENT)
    {
        events |= POLLIN;
    }
    if (channel->events & WRITE_EVENT)
    {
        events |= POLLOUT;
    }

    int i = 0;
    for (i = 0; i < FD_MAX; i++)
    {
        if (data->fds[i].fd == channel->fd)
        {
            data->fds[i].events = events;
            break;
        }
    }
    if (i >= FD_MAX)
    {
        return -1;
    }
    return 0;
}

static int poll_dispatch(eventloop_t *ev_loop, int timeout)
{
    poll_data_t *data = (poll_data_t *)ev_loop->dispatcher_data;
    int count = poll(data->fds, data->max_fd + 1, timeout * 1000);
    if (count == -1)
    {
        perror("Error poll()");
        return -1;
    }
    for (int i = 0; i <= data->max_fd; i++)
    {
        if (data->fds[i].fd == -1)
        {
            continue;
        }
        if (data->fds[i].revents & POLLIN)
        {
            event_activate(ev_loop, data->fds[i].fd, READ_EVENT);
        }
        if (data->fds[i].revents & POLLOUT)
        {
            event_activate(ev_loop, data->fds[i].fd, WRITE_EVENT);
        }
    }
    return 0;
}

static int poll_clear(eventloop_t *ev_loop)
{
    poll_data_t *data = (poll_data_t *)ev_loop->dispatcher_data;
    free(data);
    return 0;
}
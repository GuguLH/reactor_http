#include <sys/epoll.h>
#include "dispatcher.h"

#define EV_MAX 512

typedef struct ep_data
{
    int epfd;
    struct epoll_event *events;
} ep_data_t;

static void *epoll_init();
static int epoll_add(channel_t *channel, eventloop_t *ev_loop);
static int epoll_remove(channel_t *channel, eventloop_t *ev_loop);
static int epoll_modify(channel_t *channel, eventloop_t *ev_loop);
static int epoll_dispatch(eventloop_t *ev_loop, int timeout);
static int epoll_clear(eventloop_t *ev_loop);

dispatcher_t epoll_dispatcher =
    {
        epoll_init,
        epoll_add,
        epoll_remove,
        epoll_modify,
        epoll_dispatch,
        epoll_clear};

static void *epoll_init()
{
    ep_data_t *data = (ep_data_t *)malloc(sizeof(ep_data_t));
    data->epfd = epoll_create(1);
    if (data->epfd == -1)
    {
        perror("Error epoll_create()");
        return NULL;
    }
    data->events = (struct epoll_event *)calloc(EV_MAX, sizeof(struct epoll_event));
    return data;
}

static int epoll_control(channel_t *channel, eventloop_t *ev_loop, int op)
{
    ep_data_t *data = (ep_data_t *)ev_loop->dispatcher_data;

    struct epoll_event ev;
    ev.data.fd = channel->fd;
    int events = 0;
    if (channel->events & READ_EVENT)
    {
        events |= EPOLLIN;
    }
    if (channel->events & WRITE_EVENT)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
    return ret;
}

static int epoll_add(channel_t *channel, eventloop_t *ev_loop)
{
    int ret = epoll_control(channel, ev_loop, EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("Error epoll_add()");
        return -1;
    }
    return ret;
}

static int epoll_remove(channel_t *channel, eventloop_t *ev_loop)
{
    int ret = epoll_control(channel, ev_loop, EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("Error epoll_remove()");
        return -1;
    }
    // 通过channel释放对应的TcpConnection资源
    channel->destory_callback(channel->arg);
    return ret;
}

static int epoll_modify(channel_t *channel, eventloop_t *ev_loop)
{
    int ret = epoll_control(channel, ev_loop, EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("Error epoll_modify()");
        return -1;
    }
    return ret;
}

static int epoll_dispatch(eventloop_t *ev_loop, int timeout)
{
    ep_data_t *data = (ep_data_t *)ev_loop->dispatcher_data;
    int count = epoll_wait(data->epfd, data->events, EV_MAX, timeout * 1000);
    for (int i = 0; i < count; i++)
    {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        if ((events & EPOLLERR) || (events & EPOLLHUP))
        {
            // 对方断开了连接,删除fd
            // epoll_remove()
            continue;
        }
        if (events & EPOLLIN)
        {
            event_activate(ev_loop, fd, READ_EVENT);
        }
        if (events & EPOLLOUT)
        {
            event_activate(ev_loop, fd, WRITE_EVENT);
        }
    }
    return 0;
}

static int epoll_clear(eventloop_t *ev_loop)
{
    ep_data_t *data = (ep_data_t *)ev_loop->dispatcher_data;
    free(data->events);
    close(data->epfd);
    free(data);
}
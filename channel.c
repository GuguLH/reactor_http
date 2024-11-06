#include "channel.h"

channel_t *channel_init(int fd, int events, handle_func read_func, handle_func write_func, handle_func destory_func, void *arg)
{
    channel_t *channel = (channel_t *)malloc(sizeof(channel_t));
    channel->fd = fd;
    channel->events = events;
    channel->read_callback = read_func;
    channel->write_callback = write_func;
    channel->destory_callback = destory_func;
    channel->arg = arg;
    return channel;
}

void write_event_enable(channel_t *channel, bool flag)
{
    if (flag)
    {
        channel->events |= WRITE_EVENT;
    }
    else
    {
        channel->events &= ~WRITE_EVENT;
    }
}

bool is_write_event_enable(channel_t *channel)
{
    return channel->events & WRITE_EVENT;
}
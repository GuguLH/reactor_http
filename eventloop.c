#include "eventloop.h"

// 本地写数据
void task_week_up(eventloop_t *ev_loop)
{
    const char *msg = "Ping";
    write(ev_loop->socket_pair[0], msg, strlen(msg));
    printf("Write: Ping\n");
}

// 本地读数据
int read_local_msg(void *arg)
{
    eventloop_t *ev_loop = (eventloop_t *)arg;
    char buf[64] = {0};
    read(ev_loop->socket_pair[1], buf, sizeof(buf));
    printf("Read: pong\n");
    return 0;
}

eventloop_t *eventloop_init()
{
    return eventloop_init_wp(NULL);
}

eventloop_t *eventloop_init_wp(const char *thread_name)
{
    eventloop_t *ev_loop = (eventloop_t *)malloc(sizeof(eventloop_t));
    ev_loop->is_quit = false;
    ev_loop->thread_id = pthread_self();
    pthread_mutex_init(&ev_loop->mutex, NULL);
    strcpy(ev_loop->thread_name, thread_name == NULL ? "MainThread" : thread_name);
    ev_loop->dispatcher = &epoll_dispatcher;
    ev_loop->dispatcher_data = ev_loop->dispatcher->init();
    // 链表初始化
    ev_loop->head = ev_loop->tail = NULL;
    // map
    ev_loop->channel_map = channel_map_init(128);
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, ev_loop->socket_pair);
    if (ret == -1)
    {
        perror("Error socketpair()");
        exit(EXIT_FAILURE);
    }
    // fds: 0 send, 1 recv
    channel_t *channel = channel_init(ev_loop->socket_pair[1], READ_EVENT, read_local_msg, NULL, NULL, ev_loop);
    // channel 添加到任务队列
    eventloop_add_task(ev_loop, channel, ADD);
    return ev_loop;
}

int eventloop_run(eventloop_t *ev_loop)
{
    assert(ev_loop != NULL);
    // 取出事件分发和检测模型
    dispatcher_t *dispatcher = ev_loop->dispatcher;
    // 比较线程ID是否正常
    if (ev_loop->thread_id != pthread_self())
    {
        return -1;
    }
    // 循环进行事件处理
    while (!ev_loop->is_quit)
    {
        dispatcher->dispatch(ev_loop, 3);
        eventloop_process_task(ev_loop);
    }
    return 0;
}

int event_activate(eventloop_t *ev_loop, int fd, int event)
{
    if (fd < 0 || ev_loop == NULL)
    {
        return -1;
    }
    // 取出channel
    channel_t *channel = ev_loop->channel_map->list[fd];
    assert(channel->fd == fd);
    if ((event & READ_EVENT) && channel->read_callback)
    {
        channel->read_callback(channel->arg);
    }
    if ((event & WRITE_EVENT) && channel->write_callback)
    {
        channel->write_callback(channel->arg);
    }
    return 0;
}

int eventloop_add_task(eventloop_t *ev_loop, channel_t *channel, int type)
{
    // 锁
    pthread_mutex_lock(&ev_loop->mutex);
    channel_ele_t *ele = (channel_ele_t *)malloc(sizeof(channel_ele_t));
    ele->channel = channel;
    ele->type = type;
    ele->next = NULL;
    if (ev_loop->head == NULL)
    {
        ev_loop->head = ev_loop->tail = ele;
    }
    else
    {
        ev_loop->tail->next = ele;
        ev_loop->tail = ele;
    }
    pthread_mutex_unlock(&ev_loop->mutex);

    // 处理节点
    /*
    细节:
    1.对于链表节点的添加: 可能是当前线程也可能时其他线程(主线程)
        a.修改fd事件,子线程发起,当前子线程处理
        b.添加新的fd,添加任务节点是由主线程发起
    2.主线程不会处理节点,只让子线程处理节点
    */
    if (ev_loop->thread_id == pthread_self())
    {
        // 为子线程
        eventloop_process_task(ev_loop);
    }
    else
    {
        // 主线程:告诉子线程处理任务队列中的任务
        // 1.子线程正在工作 2.子线程被阻塞了
        task_week_up(ev_loop);
    }
    return 0;
}

int eventloop_process_task(eventloop_t *ev_loop)
{
    pthread_mutex_lock(&ev_loop->mutex);
    channel_ele_t *head = ev_loop->head;
    while (head != NULL)
    {
        channel_t *channel = head->channel;
        if (head->type == ADD)
        {
            eventloop_add(ev_loop, channel);
        }
        else if (head->type == DELETE)
        {
            eventloop_remove(ev_loop, channel);
        }
        else if (head->type == MODIFY)
        {
            eventloop_modify(ev_loop, channel);
        }

        channel_ele_t *tmp = head;
        head = head->next;
        free(tmp);
    }
    ev_loop->head = ev_loop->tail = NULL;
    pthread_mutex_unlock(&ev_loop->mutex);
}

int eventloop_add(eventloop_t *ev_loop, channel_t *channel)
{
    int fd = channel->fd;
    channel_map_t *channel_map = ev_loop->channel_map;
    if (fd >= channel_map->size)
    {
        // 没有足够的空间
        if (!channel_map_resize(channel_map, fd))
        {
            return -1;
        }
    }
    // 找到fd对应的数组元素位置,并存储
    if (channel_map->list[fd] == NULL)
    {
        channel_map->list[fd] = channel;
        ev_loop->dispatcher->add(channel, ev_loop);
    }
    return 0;
}

int eventloop_remove(eventloop_t *ev_loop, channel_t *channel)
{
    int fd = channel->fd;
    channel_map_t *channel_map = ev_loop->channel_map;
    if (fd >= channel_map->size)
    {
        return -1;
    }
    int ret = ev_loop->dispatcher->remove(channel, ev_loop);
    return ret;
}

int eventloop_modify(eventloop_t *ev_loop, channel_t *channel)
{
    int fd = channel->fd;
    channel_map_t *channel_map = ev_loop->channel_map;
    if ((fd >= channel_map->size) || channel_map->list[fd] == NULL)
    {
        return -1;
    }
    int ret = ev_loop->dispatcher->modify(channel, ev_loop);
    return ret;
}

int destory_channel(eventloop_t *ev_loop, channel_t *channel)
{
    // 删除channel和fd的对应关系
    ev_loop->channel_map->list[channel->fd] = NULL;
    // 关闭fd
    close(channel->fd);
    // 释放channel
    free(channel);
    return 0;
}
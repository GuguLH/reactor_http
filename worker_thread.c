#include "worker_thread.h"

void *do_handle(void *arg)
{
    worker_thread_t *thread = (worker_thread_t *)arg;
    pthread_mutex_lock(&thread->mutex);
    thread->ev_loop = eventloop_init_wp(thread->name);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
    eventloop_run(thread->ev_loop);
    return NULL;
}

int worker_thread_init(worker_thread_t *thread, int index)
{
    thread->ev_loop = NULL;
    thread->thread_id = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}

void worker_thread_run(worker_thread_t *thread)
{
    // 创建子线程
    pthread_create(&thread->thread_id, NULL, do_handle, thread);
    // 阻塞主线程,让当前函数不会直接结束
    pthread_mutex_lock(&thread->mutex);
    while (thread->ev_loop == NULL)
    {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}
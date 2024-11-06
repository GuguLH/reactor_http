#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "eventloop.h"
#include "worker_thread.h"

typedef struct thread_pool
{
    eventloop_t *main_loop;
    bool is_start;
    int thread_num;
    worker_thread_t *worker_threads;
    int index;
} thread_pool_t;

// 初始化
thread_pool_t *thread_pool_init(eventloop_t *main_loop, int count);
// 启动线程池
void thread_pool_run(thread_pool_t *pool);
// 取出线程池中某个子线程的反应堆实例
eventloop_t *take_worker_eventloop(thread_pool_t *pool);

#endif
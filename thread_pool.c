#include "thread_pool.h"

thread_pool_t *thread_pool_init(eventloop_t *main_loop, int count)
{
    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    pool->index = 0;
    pool->is_start = false;
    pool->main_loop = main_loop;
    pool->thread_num = count;
    pool->worker_threads = (worker_thread_t *)malloc(sizeof(worker_thread_t) * count);
    return pool;
}

void thread_pool_run(thread_pool_t *pool)
{
    assert(pool && !pool->is_start);
    if (pool->main_loop->thread_id != pthread_self())
    {
        exit(EXIT_FAILURE);
    }
    pool->is_start = true;
    if (pool->thread_num)
    {
        for (int i = 0; i < pool->thread_num; i++)
        {
            worker_thread_init(&pool->worker_threads[i], i);
            worker_thread_run(&pool->worker_threads[i]);
        }
    }
}

eventloop_t *take_worker_eventloop(thread_pool_t *pool)
{
    assert(pool->is_start);
    if (pool->main_loop->thread_id != pthread_self())
    {
        exit(EXIT_FAILURE);
    }
    // 从线程池中找一个子线程,然后取出里面的反应堆实例
    eventloop_t *ev_loop = pool->main_loop;
    if (pool->thread_num > 0)
    {
        ev_loop = pool->worker_threads[pool->index].ev_loop;
        pool->index = ++pool->index % pool->thread_num;
    }
    return ev_loop;
}
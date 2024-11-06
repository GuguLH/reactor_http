#ifndef __WORKER_THREAD_H__
#define __WORKER_THREAD_H__

#include <pthread.h>
#include "eventloop.h"

typedef struct worker_thread
{
    pthread_t thread_id;
    char name[24];
    pthread_mutex_t mutex; // 互斥锁
    pthread_cond_t cond;   // 条件变量
    eventloop_t *ev_loop;
} worker_thread_t;

// 初始化
int worker_thread_init(worker_thread_t *thread, int index);
// 启动
void worker_thread_run(worker_thread_t *thread);

#endif
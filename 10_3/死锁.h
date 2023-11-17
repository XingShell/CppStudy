//
// Created by 谢泽星 on 2023/11/16.
//

#ifndef MASTERCPPPLAN_死锁_H
#define MASTERCPPPLAN_死锁_H

#define _GNU_SOURCE

#include <dlfcn.h>
#include <csignal>
#include <thread>
#include <cstdio>

#include "graph.h"

typedef int (*pthread_mutex_lock_t)(pthread_mutex_t *mutex);

typedef int (*pthread_mutex_unlock_t)(pthread_mutex_t *mutex);

// 定义目标函数
pthread_mutex_lock_t pthread_mutex_lock_f;
pthread_mutex_unlock_t pthread_mutex_unlock_f;

void init_hook(void) {
    // 定义函数实现
    pthread_mutex_lock_f = (pthread_mutex_lock_t) dlsym(RTLD_NEXT, "pthread_mutex_lock");
    pthread_mutex_unlock_f = (pthread_mutex_lock_t) dlsym(RTLD_NEXT, "pthread_mutex_unlock");
}

int search_lock(uint64 lock) {

    int i = 0;

    for (i = 0; i < tg->lockidx; i++) {

        if (tg->locklist[i].lock_id == lock) {
            return i;
        }
    }

    return -1;
}

int search_empty_lock(uint64 lock) {

    int i = 0;

    for (i = 0; i < tg->lockidx; i++) {

        if (tg->locklist[i].lock_id == 0) {
            return i;
        }
    }

    return tg->lockidx;

}

void inc(int *value, int add) {

    int old = *value;
    int new_value = old+add;

    while (!__sync_bool_compare_and_swap(value, old, new_value)) {
       old = *value;
       new_value = old+add;
    }

    return ;
}

void lock_before(uint64 thread_id, uint64 lockaddr) {
    int idx = 0;
    // list<threadid, toThreadid>
    for (idx = 0; idx < tg->lockidx; idx++) {
        if ((tg->locklist[idx].lock_id == lockaddr)) {

            struct source_type from;
            from.id = thread_id;
            from.type = PROCESS;
            add_vertex(from);

            struct source_type to;
            to.id = tg->locklist[idx].id;
            tg->locklist[idx].degress++;
            to.type = PROCESS;
            add_vertex(to);

            if (!verify_edge(from, to)) {
                add_edge(from, to); //
            }

        }
    }
}

void lock_after(uint64 thread_id, uint64 lockaddr) {

    int idx = 0;
    if (-1 == (idx = search_lock(lockaddr))) {  // lock list opera

        int eidx = search_empty_lock(lockaddr);

        tg->locklist[eidx].id = thread_id;
        tg->locklist[eidx].lock_id = lockaddr;

        inc(&tg->lockidx, 1);

    } else {
        struct source_type from;
        from.id = thread_id;
        from.type = PROCESS;

        struct source_type to;
        to.id = tg->locklist[idx].id;
        tg->locklist[idx].degress--;
        to.type = PROCESS;

        if (verify_edge(from, to))
            remove_edge(from, to);

        tg->locklist[idx].id = thread_id;

    }

}

void unlock_after(uint64 thread_id, uint64 lockaddr) {
    int idx = search_lock(lockaddr);
    if (tg->locklist[idx].degress == 0) {
        tg->locklist[idx].id = 0;
        tg->locklist[idx].lock_id = 0;
        //inc(&tg->lockidx, -1);
    }
}


// 覆盖系统函数
int pthread_mutex_lock(pthread_mutex_t *mutex) {
    pthread_t selfid = pthread_self();
    lock_before(selfid, (uint64) mutex);
    pthread_mutex_lock_f(mutex);
    lock_after(selfid, (uint64) mutex);
    printf("pthread_mutex_lock: %ld, %p\n", selfid, mutex);
}

// 覆盖系统函数
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_t selfid = pthread_self();
    pthread_mutex_unlock_f(mutex);
    unlock_after(selfid, (uint64) mutex);
    printf("pthread_mutex_unlock: %ld, %p\n", selfid, mutex);
}


pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;

void *thread_funcA(void *arg) {

    pthread_mutex_lock(&mutex1);

    sleep(1);

    pthread_mutex_lock(&mutex2);

    printf("thread_funcA\n");

    pthread_mutex_unlock(&mutex2);

    pthread_mutex_unlock(&mutex1);

}

void *thread_funcB(void *arg) {

    pthread_mutex_lock(&mutex2);

    sleep(1);

    pthread_mutex_lock(&mutex3);

    printf("thread_funcB\n");

    pthread_mutex_unlock(&mutex3);

    pthread_mutex_unlock(&mutex2);

}

void *thread_funcC(void *arg) {

    pthread_mutex_lock(&mutex3);

    sleep(1);

    pthread_mutex_lock(&mutex4);

    printf("thread_funcC\n");

    pthread_mutex_unlock(&mutex4);

    pthread_mutex_unlock(&mutex3);

}

void *thread_funcD(void *arg) {

    pthread_mutex_lock(&mutex4);

    sleep(1);
    pthread_mutex_lock(&mutex1);

    printf("thread_funcD\n");

    pthread_mutex_unlock(&mutex1);

    pthread_mutex_unlock(&mutex4);

}

void test_10_3() {
    init_hook();
    pthread_t tida, tidb, tidc, tidd;

    pthread_create(&tida, NULL, thread_funcA, NULL);
    pthread_create(&tidb, NULL, thread_funcB, NULL);
    pthread_create(&tidc, NULL, thread_funcC, NULL);
    pthread_create(&tidd, NULL, thread_funcD, NULL);

    pthread_join(tida, NULL);
    pthread_join(tidb, NULL);
    pthread_join(tidc, NULL);
    pthread_join(tidd, NULL);

}

#endif //MASTERCPPPLAN_死锁_H

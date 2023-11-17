//
// Created by 谢泽星 on 2023/11/14.
//

#ifndef MASTERCPPPLAN_原子操作CAS与锁的实现_H
#define MASTERCPPPLAN_原子操作CAS与锁的实现_H

#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include <atomic>
#include "../util/timeUtil.h"

using namespace std;

#define THREAD_SIZE 10
#define TEST_ADD_SUM 100000

pthread_mutex_t mux;

#if defined(__linux__)
pthread_spinlock_t spin_lock;

void *func_spin_lock(void *arg) {
    int *p_count = (int *) arg;

    int i = 0;
    while(i++ < TEST_ADD_SUM) {
        pthread_spin_lock(&spin_lock);
        (*p_count)++;
        pthread_spin_unlock(&spin_lock);
        usleep(1);
    }
}
#endif

void *func(void *arg) {
    int *p_count = (int *) arg;

    int i = 0;
    while (i++ < TEST_ADD_SUM) {
        // 这里拆分
        // mov [idx] %eax
        // inc %eax
        // mov %eax [idx]
        (*p_count)++;
    }
}

void *func_mutex(void *arg) {
    int *p_count = (int *) arg;

    int i = 0;
    while (i++ < TEST_ADD_SUM) {
        pthread_mutex_lock(&mux);
        (*p_count)++;
        pthread_mutex_unlock(&mux);
        usleep(1);
    }
}

// ========================= test ============

void test_thread_count_add_unlock() {
    int count = 0;
    pthread_t thread_id[THREAD_SIZE] = {0};
    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_create(&thread_id[i], NULL, func, &count);
    }

    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_join(thread_id[i], NULL);
    }

    cout << "count" << count << endl;
}

void test_thread_count_add_mutex() {
    int count = 0;
    // 必须初始化
    pthread_mutex_init(&mux, NULL);

    pthread_t thread_id[THREAD_SIZE] = {0};
    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_create(&thread_id[i], NULL, func_mutex, &count);
    }

    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_join(thread_id[i], NULL);
    }

    cout << "count=" << count << endl;
}

void test_thread_count_add_spin_lock() {
    int count = 0;
    // 必须初始化
    pthread_mutex_init(&mux, NULL);

    pthread_t thread_id[THREAD_SIZE] = {0};
    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_create(&thread_id[i], NULL, func_spin_lock, &count);
    }

    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_join(thread_id[i], NULL);
    }

    cout << "count=" << count << endl;
}

// 汇编时间原子操作加加
//int inc(int *value, int add)   {
//    int old;
//    __asm__ volatile (
//            "lock; xaddl %2, %1;"
//            : "=a" (old)
//            : "m" (*value), "a" (add)
//            : "cc", "memory"
//            );
//    return old;
//}



void *func_atomic_inc(void *arg) {
    atomic_int64_t *p_count = (atomic_int64_t *) arg;

    int i = 0;
    while (i++ < TEST_ADD_SUM) {
        (*p_count)++;
    }
}

void test_thread_count_add_atomic() {
    atomic_int64_t count = 0;
    // 必须初始化
    // pthread_mutex_init(&mux, NULL);

    pthread_t thread_id[THREAD_SIZE] = {0};
    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_create(&thread_id[i], NULL, func_atomic_inc, &count);
    }

    for (int i = 0; i < THREAD_SIZE; i++) {
        pthread_join(thread_id[i], NULL);
    }

    cout << "count=" << count << endl;
}

void test_10_1() {
    usleep(1000);

    // 不锁
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    test_thread_count_add_unlock();
    gettimeofday(&tv_end, NULL);
    cout << "unlock_time_cost=" << TIME_SUB_MS(tv_end, tv_start) << endl;

    // mutext 锁
    gettimeofday(&tv_start, NULL);
    test_thread_count_add_mutex();
    gettimeofday(&tv_end, NULL);
    cout << "mutex_time_cost=" << TIME_SUB_MS(tv_end, tv_start) << endl;

    // spain lock
    gettimeofday(&tv_start, NULL);
    test_thread_count_add_spin_lock();
    gettimeofday(&tv_end, NULL);
    cout << "spin_lock_time_cost=" << TIME_SUB_MS(tv_end, tv_start) << endl;

    // 原子操作
    gettimeofday(&tv_start, NULL);
    test_thread_count_add_atomic();
    gettimeofday(&tv_end, NULL);
    cout << "atomic_time_cost=" << TIME_SUB_MS(tv_end, tv_start) << endl;

}

#endif //MASTERCPPPLAN_原子操作CAS与锁的实现_H

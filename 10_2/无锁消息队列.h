//
// Created by 谢泽星 on 2023/11/15.
//

#ifndef MASTERCPPPLAN_无锁消息队列_H
#define MASTERCPPPLAN_无锁消息队列_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <time.h>
#include <atomic>
#include <list>
#include <memory>
#include <atomic>
#include <unistd.h>

#include "../util/timeUtil.h"

#include "ypipe.h"
#include "ArrayLockFreeQueue.h"
#include "SimpleLockFreeQueue.h"
#include "BlockQueue.h"


#define PRINT_THREAD_INTO() printf("%s %lu into\n", __FUNCTION__, pthread_self())
#define PRINT_THREAD_LEAVE() printf("%s %lu leave\n", __FUNCTION__, pthread_self())

static int s_queue_item_num = 50000000; // 每个线程插入的元素个数
static int s_producer_thread_num = 1;  // 生产者线程数量
static int s_consumer_thread_num = 1;  // 消费线程数量

static atomic_int64_t s_count_push_atomic = 0;
static int s_count_push = 0;
static int s_count_pop = 0;
static atomic_int64_t s_count_pop_atomic = 0;

// 一写一读无锁队列，(front ,r) 判断队列是否可读
// back直接写 （w, f）控制刷新
ypipe_t<int, 100> yqueue;

typedef void *(*thread_func_t)(void *argv);

static int lxx_atomic_add(atomic_int64_t *ptr, int increment) {
//    int old_value = *ptr;
//    __asm__ volatile("lock; xadd %0, %1 \n\t"
//            : "=r"(old_value), "=m"(*ptr)
//            : "0"(increment), "m"(*ptr)
//            : "cc", "memory");
    *ptr += increment;
    return *ptr;
}

// 有锁队列，直接使用list
static std::list<int> s_list;
static std::mutex s_mutex;

void *mutexqueue_producer_thread(void *argv) {
    // PRINT_THREAD_INTO();
    for (int i = 1; i <= s_queue_item_num; i++) {
        s_mutex.lock();
        s_count_push++;
        s_list.emplace_back(s_count_push);
        s_mutex.unlock();
    }
    // PRINT_THREAD_LEAVE();
    return NULL;
}

void *mutexqueue_consumer_thread(void *argv) {
    int value = 0;
    int last_value = 0;
    int nodata = 0;
    // PRINT_THREAD_INTO();
    while (true) {
        s_mutex.lock();
        if (s_list.size() > 0) {
            value = s_list.front();
            s_list.pop_front();
            last_value = value;
            s_count_pop++;
            nodata = 0;
        } else {
            nodata = 1;
        }
        s_mutex.unlock();
        if (nodata) {
            // usleep(1000);
            // 没有数据可以消费，让出cpu
            sched_yield();
        }
        if (s_count_pop >= s_queue_item_num * s_producer_thread_num) {
            // 消费完毕
            break;
        }
    }
//    printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__, last_value, s_count_pop, s_queue_item_num,
//           s_consumer_thread_num);
    // PRINT_THREAD_LEAVE();
    return NULL;
}


void *yqueue_producer_thread(void *argv) {
    int count = 0;
    for (int i = 1; i <= s_queue_item_num; i++) {
        s_count_push_atomic++;
        yqueue.write(s_count_push_atomic.load(), false); // enqueue的顺序是无法保证的，我们只能计算enqueue的个数
        yqueue.flush();
    }
    return NULL;
}

void *yqueue_consumer_thread(void *argv) {
    int last_value = 0;

    while (true) {
        int value = 0;
        if (yqueue.read(&value)) {
            s_count_pop_atomic++;
            last_value = value;
        } else {
            sched_yield();
        }

        if (s_count_pop_atomic >= s_queue_item_num * s_producer_thread_num) {
            s_count_pop = s_count_pop_atomic.load();
            s_count_push = s_count_push_atomic.load();
            break;
        }
    }
    return NULL;
}


static BlockQueue s_blockqueue;

void *blockqueue_producer_thread(void *argv) {
    // PRINT_THREAD_INTO();
    int count = 0;
    for (int i = 0; i < s_queue_item_num; i++) {
        s_count_push_atomic++;
        // count = lxx_atomic_add(&s_count_push_atomic, 1); // 只是保证enqueue数据唯一性
        s_blockqueue.Enqueue(s_count_push_atomic);              // enqueue的顺序是无法保证的，我们只能计算enqueue的个数
    }
    // PRINT_THREAD_LEAVE();
    return NULL;
}

// 条件变量唤醒优于yield usleep
// wait 方法会主动的释放锁，而 sleep 方法则不会
void *blockqueue_consumer_thread(void *argv) {
    int last_value = 0;
    // PRINT_THREAD_INTO();

    while (true) {
        int value = 0;
        if (s_blockqueue.Dequeue(value, 30) == 1) {
            s_count_pop_atomic++;
            // lxx_atomic_add(&s_count_pop_atomic, 1);
        }
        if (s_count_pop_atomic >= s_queue_item_num * s_producer_thread_num) {
            break;
        }
    }
    s_count_pop = s_count_pop_atomic.load();
    s_count_push = s_count_push_atomic.load();
    // PRINT_THREAD_LEAVE();
    return NULL;
}


static SimpleLockFreeQueue<int> s_lockfreequeue;
void *lockfreequeue_producer_thread(void *argv) {
    int count = 0;
    for (int i = 1; i <= s_queue_item_num; i++) {
        s_count_push_atomic++;
        s_lockfreequeue.enqueue(s_count_push_atomic.load());
        // enqueue的顺序是无法保证的，我们只能计算enqueue的个数
    }
    return NULL;
}

void *lockfreequeue_consumer_thread(void *argv) {
    int last_value = 0;
    static int s_pid_count = 0;
    s_pid_count++;
    // int pid = s_pid_count;
    while (true) {
        int value = 0;
        if (s_lockfreequeue.try_dequeue(value)) {
            s_count_pop_atomic++;
            last_value = value;
        } else {
            // printf("pid:%d, null\n", pid);
            // usleep(10);
            sched_yield();
        }

        if (s_count_pop_atomic >= s_queue_item_num * s_producer_thread_num) {
            break;
        }
    }
    s_count_pop = s_count_pop_atomic.load();
    s_count_push = s_count_push_atomic.load();
//    printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__, last_value, s_count_pop, s_queue_item_num,
//           s_consumer_thread_num);

    return NULL;
}




ArrayLockFreeQueue<int> arraylockfreequeue;
void *arraylockfreequeue_producer_thread(void *argv) {

    int write_failed_count = 0;

    for (int i = 1; i <= s_queue_item_num;) {
        s_count_push_atomic++;
        if (arraylockfreequeue.enqueue(s_count_push_atomic.load())) // enqueue的顺序是无法保证的，我们只能计算enqueue的个数
        {
            i++;
        } else {
            s_count_push_atomic--;
            write_failed_count++;
            sched_yield();
        }
    }
    return NULL;
}

void *arraylockfreequeue_consumer_thread(void *argv) {
    int value = 0;
    int read_failed_count = 0;
    while (true) {
        if (arraylockfreequeue.dequeue(value)) {
            s_count_pop_atomic++;
        } else {
            read_failed_count++;
            sched_yield();
        }
        if (s_count_pop_atomic.load() >= s_queue_item_num * s_producer_thread_num) {
            // printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__, last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
            break;
        }
    }
    s_count_pop = s_count_pop_atomic.load();
    s_count_push = s_count_push_atomic.load();

    return NULL;
}

void *yqueue_producer_thread_batch(void *argv) {

    for (int i = 1; i < s_queue_item_num; i++) {
        s_count_push_atomic++;
        yqueue.write(s_count_push_atomic.load(), true);  // 写true
    }
    // 最后一次
    s_count_push_atomic++;
    yqueue.write(s_count_push_atomic.load(), false);
    yqueue.flush();
    return NULL;
}

void *yqueue_consumer_thread_yield(void *argv) {

    while (true) {
        int value = 0;
        if (yqueue.read(&value)) {
            s_count_pop_atomic++;
        } else {
            sched_yield();
        }

        if (s_count_pop_atomic >= s_queue_item_num * s_producer_thread_num) {
            s_count_pop = s_count_pop_atomic.load();
            s_count_push = s_count_push_atomic.load();
            break;
        }
    }

    return NULL;
}

std::mutex ypipe_mutex_;
std::condition_variable ypipe_cond_;

void *yqueue_producer_thread_condition(void *argv) {
    PRINT_THREAD_INTO();
    int count = 0;
    for (int i = 0; i < s_queue_item_num;) {
        yqueue.write(count, false); // enqueue的顺序是无法保证的，我们只能计算enqueue的个数
        count = lxx_atomic_add(&s_count_push_atomic, 1);
        i++;
        // yqueue.flush();
        // std::unique_lock<std::mutex> lock(ypipe_mutex_);
        //  ypipe_cond_.notify_one();
        if (!yqueue.flush()) {
            // printf("notify_one\n");
            std::unique_lock<std::mutex> lock(ypipe_mutex_);
            ypipe_cond_.notify_one();
        }
    }
    std::unique_lock<std::mutex> lock(ypipe_mutex_);
    ypipe_cond_.notify_one();
    PRINT_THREAD_LEAVE();
    return NULL;
}

void *yqueue_consumer_thread_condition(void *argv) {
    int last_value = 0;
    PRINT_THREAD_INTO();

    while (true) {
        int value = 0;
        if (yqueue.read(&value)) {
            if (s_consumer_thread_num == 1 && s_producer_thread_num == 1 &&
                (last_value + 1) != value) // 只有一入一出的情况下才有对比意义
            {
                // printf("pid:%lu, -> value:%d, expected:%d\n", pthread_self(), value, last_value + 1);
            }
            lxx_atomic_add(&s_count_pop_atomic, 1);
            last_value = value;
        } else {
            // printf("%s %lu no data, s_count_pop:%d\n", __FUNCTION__, pthread_self(), s_count_pop);
            // usleep(100);
            std::unique_lock<std::mutex> lock(ypipe_mutex_);
            //  printf("wait\n");
            ypipe_cond_.wait(lock);
            // sched_yield();
        }

        if (s_count_pop >= s_queue_item_num * s_producer_thread_num) {
            // printf("%s dequeue:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__, last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
            break;
        }
    }
    printf("%s dequeue: last_value:%d, s_count_pop:%d, %d, %d\n", __FUNCTION__,
           last_value, s_count_pop, s_queue_item_num, s_consumer_thread_num);
    PRINT_THREAD_LEAVE();
    return NULL;
}

int test_queue(thread_func_t func_push, thread_func_t func_pop, char **argv) {
    int64_t start = get_current_millisecond();

    pthread_t tid_push[s_producer_thread_num] = {0};
    for (int i = 0; i < s_producer_thread_num; i++) {
        int ret = pthread_create(&tid_push[i], NULL, func_push, argv);
        if (0 != ret) {
            printf("create thread failed\n");
        }
    }

    pthread_t tid_pop[s_consumer_thread_num] = {0};
    for (int i = 0; i < s_consumer_thread_num; i++) {
        int ret = pthread_create(&tid_pop[i], NULL, func_pop, argv);
        if (0 != ret) {
            printf("create thread failed\n");
        }
    }

    for (int i = 0; i < s_producer_thread_num; i++) {
        pthread_join(tid_push[i], NULL);
    }
    for (int i = 0; i < s_consumer_thread_num; i++) {
        pthread_join(tid_pop[i], NULL);
    }

    int64_t end = get_current_millisecond();
    int64_t temp = s_count_push;
    int64_t ops = (temp * 1000) / (end - start);

    printf("spend time : %ldms\t, push:%d, pop:%d, ops:%lu\n", (end - start), s_count_push, s_count_pop,
           ops);
    return 0;
}

int init_emty() {
    s_count_push = 0;
    s_count_pop = 0;
    s_count_push_atomic = 0;
    s_count_pop_atomic = 0;
}

int test_unlock_queue() {
    s_consumer_thread_num = 5;
    s_producer_thread_num = 2;
    printf("\nthread num - producer:%d, consumer:%d,  push:%d\n\n", s_producer_thread_num, s_consumer_thread_num,
           s_queue_item_num);
    for (int i = 0; i < 1; i++) {

        init_emty();
        printf("use mutexqueue ----------->\n");

        test_queue(mutexqueue_producer_thread, mutexqueue_consumer_thread, NULL);

        init_emty();
        printf("\nuse blockqueue ----------->\n");
        test_queue(blockqueue_producer_thread, blockqueue_consumer_thread, NULL);

        init_emty();
        printf("\nuse lockfreequeue ----------->\n"); // 指针
        test_queue(lockfreequeue_producer_thread, lockfreequeue_consumer_thread, NULL);

        if (s_consumer_thread_num == 1 && s_producer_thread_num == 1) {
            init_emty();
            printf("\nuse ypipe_t ----------->\n");
            test_queue(yqueue_producer_thread, yqueue_consumer_thread, NULL);

            init_emty();
            printf("\nuse ypipe_t batch ----------->\n");
            test_queue(yqueue_producer_thread_batch, yqueue_consumer_thread_yield, NULL);
        }

        init_emty();
        printf("\nuse arraylockfreequeue ----------->\n");
        test_queue(arraylockfreequeue_producer_thread, arraylockfreequeue_consumer_thread, NULL);
#if 0


        if (s_consumer_thread_num == 1 && s_producer_thread_num == 1) {
            s_count_push = 0;
            s_count_pop = 0;
            printf("\nuse ypipe_t ----------->\n");
            test_queue(yqueue_producer_thread, yqueue_consumer_thread, NULL);
            s_count_push = 0;
            s_count_pop = 0;
            printf("\nuse ypipe_t batch ----------->\n");
            test_queue(yqueue_producer_thread_batch, yqueue_consumer_thread_yield, NULL);

            s_count_push = 0;
            s_count_pop = 0;
            printf("\nuse ypipe_t condition ----------->\n");
            test_queue(yqueue_producer_thread_condition, yqueue_consumer_thread_condition, NULL);
        } else {
            printf("\nypipe_t only support one write one read thread, bu you write %d thread and read %d thread so no test it ----------->\n",
                   s_producer_thread_num, s_consumer_thread_num);
        }
#endif
    }

    printf("finish\n");
    return 0;
}

void test_10_2() {
    test_unlock_queue();
}

#endif //MASTERCPPPLAN_无锁消息队列_H

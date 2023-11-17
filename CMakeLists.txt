cmake_minimum_required(VERSION 3.22)
project(masterCppPlan)

set(CMAKE_CXX_STANDARD 17)

add_executable(masterCppPlan main.cpp 10_1/原子操作CAS与锁的实现.h util/timeUtil.h util/systemUtil.h 10_1/CPU亲缘.h util/mmapUse.h 10_2/无锁消息队列.h 10_2/yqueue.h 10_2/ypipe.h 10_2/atomic_ptr.h 10_2/ArrayLockFreeQueue.h
        10_2/ArrayLockFreeQueueImpl.h 10_2/atom_opt.h 10_2/SimpleLockFreeQueue.h 10_2/BlockQueue.h 10_3/死锁.h 10_3/graph.h 10_4/内存泄露检测.h 10_5/分布式锁.h)
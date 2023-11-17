//
// Created by 谢泽星 on 2023/11/15.
//

#ifndef MASTERCPPPLAN_CPU亲缘_H
#define MASTERCPPPLAN_CPU亲缘_H

#include <csignal>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>

void process_affinity(int num) {


    //pid_t self_id = syscall(__NR_gettid);
    pid_t self_id = getpid();

    //fd_set
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(self_id % num, &mask);

    sched_setaffinity(self_id, sizeof(mask), &mask);

    while(1) usleep(1);
}

#endif //MASTERCPPPLAN_CPU亲缘_H

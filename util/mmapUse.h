//
// Created by 谢泽星 on 2023/11/15.
//

#ifndef MASTERCPPPLAN_MMAPUSE_H
#define MASTERCPPPLAN_MMAPUSE_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int useMmap() {

    int fd = open("./affinity.c", O_RDWR);

    //read(); write()

    unsigned char *addr = (unsigned char *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    printf("affinity: %s\n", addr);

    int i = 0;

    for (i = 0;i < 20;i ++) {
        *(addr+i) = 'A' + i;
    }

}

#endif //MASTERCPPPLAN_MMAPUSE_H

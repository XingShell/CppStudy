//
// Created by 谢泽星 on 2023/11/17.
//

#ifndef MASTERCPPPLAN_内存泄露检测_H
#define MASTERCPPPLAN_内存泄露检测_H

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <mcheck.h>
#include <link.h>

size_t Convert2ELF(void *addr) {
    Dl_info info;
    struct link_map *link;
    dladdr1(addr, &info, (void **)(&link), RTLD_DL_LINKMAP);
    return (size_t) addr - link->l_addr;
}

#if 0
void *_malloc(size_t size, const char *filename, int line) {
    void *p = malloc(size);

//    char buff[128] = {0};
//    sprintf(buff, "./mem/%p.mem", p);
//
//    FILE *fp = fopen(buff, "w");
//    fprintf(fp, "[+]%s:%d, addr: %p, size: %ld\n",
//            filename, line, p, size);
//
//    fflush(fp);
//    fclose(fp);
    printf("[+]%s:%d, %p\n", filename, line, p);
    return p;
}

void _free(void *p, const char *filename, int line) {
    printf("[-]%s:%d, %p\n", filename, line, p);

//    char buff[128] = {0};
//    sprintf(buff, "./mem/%p.mem", p);
//    if (unlink(buff) < 0) {
//        printf("double free: %p\n", p);
//        return ;
//    }
    return free(p);
}

#define malloc(size)   _malloc(size, __FILE__, __LINE__)

#define free(size)   _free(size, __FILE__, __LINE__)

#else
#define HookMalloc
typedef void *(*malloc_t)(size_t size);
typedef void (*free_t)(void *ptr);

malloc_t malloc_f = NULL;
free_t free_f = NULL;

static void init_hook(void) {

	if (malloc_f == NULL) {
		malloc_f = (malloc_t)dlsym(RTLD_NEXT, "malloc");
	}


	if (free_f == NULL) {
		free_f = (free_t)dlsym(RTLD_NEXT, "free");
	}
}

int enable_malloc_hook = 1;
int enable_free_hook = 1;
// 可以改名malloc hook
void *malloc(size_t size) {
    // printf 会调用malloc
    void *p = nullptr;
    if(malloc_f) {
        p = malloc_f(size);
    }
    if(enable_malloc_hook) {
        enable_malloc_hook = 0;

        // 虚拟内存地址
        void *caller = __builtin_return_address(0);
        printf("[+%p]-[%p]-size: %ld\n", Convert2ELF(caller), p, size);

        enable_malloc_hook = 1;
    }
    return p;


    return NULL;
}


void free(void *ptr) {
    if(enable_free_hook) {
        enable_free_hook = 0;
        printf("ptr: %p\n", ptr);
        enable_free_hook = 1;
    }

    if(free_f) {
        free_f(ptr);
    }
}


#endif

void test_10_4() {

    // mtrace();
    // setenv("MALLOC_TRACE", "/tmp/tmp.wHgMUUjupg/cmake-build-debug/mem.txt", 1);
#ifdef HookMalloc
    init_hook();
#endif
    void *p1 = malloc(10);  //_malloc(size, __FILE__, __LINE__)
    void *p2 = malloc(15);
    void *p3 = malloc(20);

    free(p2);
    free(p3);

    // muntrace();
}

#endif //MASTERCPPPLAN_内存泄露检测_H

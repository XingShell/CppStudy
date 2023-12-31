//
// Created by 谢泽星 on 2023/11/16.
//

#ifndef MASTERCPPPLAN_ATOMIC_PTR_H
#define MASTERCPPPLAN_ATOMIC_PTR_H

#ifndef __ZMQ_ATOMIC_PTR_HPP_INCLUDED__
#define __ZMQ_ATOMIC_PTR_HPP_INCLUDED__

#if defined __GNUC__
#define likely(x) __builtin_expect ((x), 1)
#define unlikely(x) __builtin_expect ((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define ZMQ_ATOMIC_PTR_MUTEX
//#if (defined __i386__ || defined __x86_64__) && defined __GNUC__
//#define ZMQ_ATOMIC_PTR_X86              // 应该走这里去执行
//#elif defined __tile__
//#define ZMQ_ATOMIC_PTR_TILE
//#elif (defined ZMQ_HAVE_SOLARIS || defined ZMQ_HAVE_NETBSD)
//#define ZMQ_ATOMIC_PTR_ATOMIC_H
//#endif


#if defined ZMQ_ATOMIC_PTR_ATOMIC_H
#include <atomic.h>
#elif defined ZMQ_ATOMIC_PTR_TILE
#include <arch/atomic.h>
#endif

#define alloc_assert(x) \
    do {\
        if (unlikely (!x)) {\
            fprintf (stderr, "FATAL ERROR: OUT OF MEMORY (%s:%d)\n",\
                __FILE__, __LINE__);\
        }\
    } while (false)


//  This class encapsulates several atomic operations on pointers.

template<typename T>
class atomic_ptr_t {
public:

    //  Initialise atomic pointer
    inline atomic_ptr_t() {
        ptr = NULL;
    }

    //  Destroy atomic pointer
    inline ~atomic_ptr_t() {
    }

    //  Set value of atomic pointer in a non-threadsafe way
    //  Use this function only when you are sure that at most one
    //  thread is accessing the pointer at the moment.
    inline void set(T *ptr_)//非原子操作
    {
        this->ptr = ptr_;
    }

    //  Perform atomic 'exchange pointers' operation. Pointer is set
    //  to the 'val' value. Old value is returned.
    // 设置新值，返回旧值
    inline T *xchg(T *val_)    //原子操
    {
        sync.lock();
        T *old = (T *) ptr;
        ptr = val_;
        sync.unlock();
        return old;
//#if defined ZMQ_ATOMIC_PTR_ATOMIC_H
//        return (T*) atomic_swap_ptr (&ptr, val_);
//#elif defined ZMQ_ATOMIC_PTR_TILE
//        return (T*) arch_atomic_exchange (&ptr, val_);
//#elif defined ZMQ_ATOMIC_PTR_X86
//        T *old;
//            __asm__ volatile (
//                "lock; xchg %0, %2"
//                : "=r" (old), "=m" (ptr)
//                : "m" (ptr), "0" (val_));
//            return old;
//#elif defined ZMQ_ATOMIC_PTR_MUTEX
//        sync.lock();
//        T *old = (T *) ptr;
//        ptr = val_;
//        sync.unlock();
//        return old;
//#else
//#error atomic_ptr is not implemented for this platform
//#endif
    }

    //  Perform atomic 'compare and swap' operation on the pointer.
    //  The pointer is compared to 'cmp' argument and if they are
    //  equal, its value is set to 'val'. Old value of the pointer
    //  is returned.

    //原子操作
    inline T *cas(T *cmp_, T *val_) {
//#if defined ZMQ_ATOMIC_PTR_ATOMIC_H
//        return (T*) atomic_cas_ptr (&ptr, cmp_, val_);
//#elif defined ZMQ_ATOMIC_PTR_TILE
//        return (T*) arch_atomic_val_compare_and_exchange (&ptr, cmp_, val_);
//#elif defined ZMQ_ATOMIC_PTR_X86
//        T *old;
//            __asm__ volatile (
//                "lock; cmpxchg %2, %3"
//                : "=a" (old), "=m" (ptr)
//                : "r" (val_), "m" (ptr), "0" (cmp_)
//                : "cc");
//            return old;
//#else
        // if ptr==cmp，ptr=val return cmp

        if (__sync_bool_compare_and_swap(&ptr, cmp_, val_)) {
            return cmp_;
        }
        return (T *)ptr;
#endif
    }

private:

    volatile T *ptr;
#if defined ZMQ_ATOMIC_PTR_MUTEX
    mutex sync;
#endif

    atomic_ptr_t(const atomic_ptr_t &);

    const atomic_ptr_t &operator=(const atomic_ptr_t &);
};


//  Remove macros local to this file.
#if defined ZMQ_ATOMIC_PTR_WINDOWS
#undef ZMQ_ATOMIC_PTR_WINDOWS
#endif
#if defined ZMQ_ATOMIC_PTR_ATOMIC_H
#undef ZMQ_ATOMIC_PTR_ATOMIC_H
#endif
#if defined ZMQ_ATOMIC_PTR_X86
#undef ZMQ_ATOMIC_PTR_X86
#endif
#if defined ZMQ_ATOMIC_PTR_ARM
#undef ZMQ_ATOMIC_PTR_ARM
#endif
#if defined ZMQ_ATOMIC_PTR_MUTEX
#undef ZMQ_ATOMIC_PTR_MUTEX
#endif



#endif //MASTERCPPPLAN_ATOMIC_PTR_H

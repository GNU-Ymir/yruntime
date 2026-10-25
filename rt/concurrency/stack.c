#ifndef _GNU_SOURCE
#  define _GNU_SOURCE // pthread_getattr_np
#endif

#include <rt/concurrency/stack.h>
#include <pthread.h>

static __thread uintptr_t __YRT_STACK_LOW__ = 0;

uint64_t _yrt_stack_left () {
    uintptr_t here = (uintptr_t) __builtin_frame_address (0);

#ifdef __linux__
    if (__YRT_STACK_LOW__ == 0) {
        pthread_attr_t attr;
        void * addr;
        size_t size;
        if (pthread_getattr_np (pthread_self (), &attr) != 0) return UINT64_MAX;

        int res = pthread_attr_getstack (&attr, &addr, &size);
        pthread_attr_destroy (&attr);
        if (res != 0 || addr == NULL) return UINT64_MAX;

        __YRT_STACK_LOW__ = (uintptr_t) addr;
    }

    return here > __YRT_STACK_LOW__ ? here - __YRT_STACK_LOW__ : 0;
#else
    (void) here;
    return UINT64_MAX;
#endif
}

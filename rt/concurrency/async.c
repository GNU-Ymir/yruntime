#include "async.h"
#include <rt/utils/gc.h>
#include <rt/concurrency/thread.h>

typedef uint8_t (*_yrt_async_resume_t) (void * frame, void * out, _yrt_delegate_t waker);

void _yrt_async_run (_yrt_delegate_t handle, void * out) {
    // in GC memory, a waker kept after the run returned can still be called
    sem_t * sem = GC_malloc (sizeof (sem_t));
    _yrt_thread_sem_init (sem, 0, 0);

    _yrt_delegate_t waker;
    waker.closure = sem;
    waker.func = (void*) &_yrt_i_async_wake;

    _yrt_async_resume_t resume = (_yrt_async_resume_t) handle.func;
    while (resume (handle.closure, out, waker)) {
        _yrt_thread_sem_wait (sem);
    }
}

void _yrt_i_async_wake (void * sem) {
    _yrt_thread_sem_post ((sem_t*) sem);
}

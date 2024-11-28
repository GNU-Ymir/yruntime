#include "future.h"
#include <rt/utils/gc.h>
#include <threads.h>
#include <rt/except/panic.h>

thread_local _yrt_thread_t selfId;

_yrt_future_t _yrt_spawn_future (_yrt_lazy_closure_t closure, uint32_t valueSize) {
    _yrt_future_t result;

    result.content = GC_malloc (sizeof (_yrt_future_content_t));
    _yrt_thread_sem_init (&result.content-> wait, 0, 0);
    _yrt_thread_mutex_init (&result.content-> mutex, NULL);

    result.content-> finished = 0;
    result.content-> closure = closure;
    result.content-> valueSize = valueSize;
    result.content-> value = NULL;

    _yrt_thread_create (&result.id, NULL, &_yrt_future_main, result.content);
    _yrt_thread_sem_wait (&result.content-> wait);

    return result;
}

void* _yrt_wait_future (_yrt_future_t f) {
    if (f.content == NULL) return NULL;
    if (selfId == f.id) {
        _yrt_exc_terminate ("Waiting self thread %lx\n", __FILE__, __FUNCTION__, __LINE__);
    }

    _yrt_thread_mutex_lock (&f.content-> mutex);
    if (!f.content-> finished) {
        _yrt_thread_sem_wait (&f.content-> wait);
    }
    _yrt_thread_mutex_unlock (&f.content-> mutex);

    return f.content-> value;
}


void* _yrt_future_main (void * data) {
    _yrt_future_content_t * content = (_yrt_future_content_t*) data;
    _yrt_thread_sem_post (&content-> wait);

    if (content-> valueSize != 0) {
        // printf ("Here ?%d\n", content-> valueSize);
        void* data = GC_malloc (content-> valueSize);
        // printf ("Call ?%d\n", content-> closure.func);

        content-> closure.func (content-> closure.closure, data);
        content-> value = data;
    } else {
        void (*fptr) (void*) = (void(*)(void*)) content-> closure.func;
        fptr (content-> closure.closure);
    }

    content-> finished = 1;
    _yrt_thread_sem_post (&content-> wait);
}


uint8_t _yrt_check_finished_future (_yrt_future_t f) {
    if (f.content == NULL) return 1;

    return f.content-> finished;
}

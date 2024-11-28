#ifndef FUTURE_H_
#define FUTURE_H_

#include <rt/memory/types.h>
#include <rt/concurrency/thread.h>
#include <stdint.h>

typedef struct _yrt_future_content_t {
    struct _yrt_lazy_closure_t closure;
    sem_t wait;
    _yrt_mutex_t mutex;
    uint8_t finished;
    uint32_t valueSize;
    void * value;
} _yrt_future_content_t;


typedef struct _yrt_future_t {
    _yrt_thread_t id;
    _yrt_future_content_t * content;
} _yrt_future_t;

/**
 * Spawn a future value from a closure
 * @params:
 *    - closure: the closure to call in the future spawned value
 *    - valueSize: the size of the value returned by the closure
 */
_yrt_future_t _yrt_spawn_future (_yrt_lazy_closure_t closure, uint32_t valueSize);

/**
 * Wait for the future to be finised
 * @params:
 *   - f: the future to wait
 * @returns: the value of the future
 */
void* _yrt_wait_future (_yrt_future_t f);

/**
 * @returns: true if the future is finished
 */
uint8_t _yrt_check_finished_future (_yrt_future_t f);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =====================================          WORKER          =====================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * The main fonction spawned in a thread computing the value of the future
 */
void* _yrt_future_main (void * f);

#endif // FUTURE_H_

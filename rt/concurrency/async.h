#ifndef ASYNC_H_
#define ASYNC_H_

#include <rt/memory/types.h>
#include <stdint.h>

/**
 * Drive an async value to completion, sleeping until its waker is called whenever it is pending
 * @params:
 *    - handle: the async value, its func is `uint8_t resume (void * frame, void * out, _yrt_delegate_t waker)`
 *              returning 1 while pending, and 0 once the result is written in `out`
 *    - out: where the result of the async value is written
 */
void _yrt_async_run (_yrt_delegate_t handle, void * out);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =====================================          WAKER          ======================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * The function of the waker _yrt_async_run gives to the resume function, posts the semaphore `sem`
 */
void _yrt_i_async_wake (void * sem);

#endif // ASYNC_H_

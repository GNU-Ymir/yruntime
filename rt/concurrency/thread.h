#ifndef _THREAD_H_
#define _THREAD_H_

//#ifdef __linux__
#include <pthread.h>
#include <stdint.h>

typedef pthread_t _yrt_thread_t;
typedef pthread_attr_t _yrt_attr_t;
typedef pthread_mutex_t _yrt_mutex_t;
typedef pthread_cond_t _yrt_cond_t;
typedef pthread_condattr_t _yrt_condattr_t;
typedef pthread_mutexattr_t _yrt_mutexattr_t;
typedef pthread_barrier_t _yrt_barrier_t;

//#endif

#include <semaphore.h>


/**
 * Read data inside a Pipe
 * @params:
 *    - stream: the pipe
 *    - size: the size to read (in bytes)
 * @return: the data read
 * @warning: this function is blocking until everything is read, or the pipe is closed
 */
#ifdef _WIN32
void * _yrt_read_pipe (void * stream, unsigned long long size);
#else
void * _yrt_read_pipe (int stream, unsigned long long size);
#endif

/**
 * Write data inside a Pipe
 * @params:
 *    - stream: a pip
 *    - data: the data to write
 *    - size: the size (in bytes)
 */
#ifdef _WIN32
void _yrt_write_pipe (void * stream, void * data, unsigned long long size);
#else
void _yrt_write_pipe (int stream, void * data, unsigned long long size);
#endif

/**
 * Create a new thread that will executed a function
 * @params:
 *    - attr: the attribute of the thread (see libpthread )
 *    - call: the function to call
 *    - data: the parameters to pass to the function
 * @returns:
 *    - id: the id of the created thread
 */
void _yrt_thread_create (_yrt_thread_t * id, _yrt_attr_t* attr, void*(*call)(void*), void* data);

/**
 * Wait for the completion of a thread
 * @params:
 *    - p: the id of the thread to wait
 * @returns:
 *    - retval: the return value of the thread
 */
void _yrt_thread_join (_yrt_thread_t p, void** retval);

/**
 * Detach a thread from the main thread, but does not kill it
 * @params:
 *    - p: the id of the thread
 */
void _yrt_thread_detach (_yrt_thread_t p);

/**
 * Create a new mutex
 * @params:
 *    - data: the data of the mutex (see libpthread)
 * @returns:
 *    - lock: the id of the lock
 */
void _yrt_thread_mutex_init (_yrt_mutex_t* lock, _yrt_mutexattr_t * data);

/**
 * Lock a mutex
 * @params:
 *    - lock: the mutex to lock
 */
void _yrt_thread_mutex_lock (_yrt_mutex_t* lock);

/**
 * Unlock a mutex
 * @params:
 *    - lock: the mutex to unlock
 */
void _yrt_thread_mutex_unlock (_yrt_mutex_t* lock);

/**
 * Create a new barrier blocking nb thread before opening
 */
void _yrt_thread_barrier_init (_yrt_barrier_t * loc, uint32_t nb);

/**
 * Wait for the barrier to open in a thread
 */
void _yrt_thread_barrier_wait (_yrt_barrier_t * loc);

/**
 * Destroy a barrier
 */
void _yrt_thread_barrier_destroy (_yrt_barrier_t * loc);

/**
 * Init a waitable conditional
 * @params:
 *    - data: the data of the condition (see libpthread)
 * @returns:
 *    - cond: the condition
 */
void _yrt_thread_cond_init (_yrt_cond_t * cond, _yrt_condattr_t* data);

/**
 * Wait for a condition to be triggered
 * @params:
 *    - cond: the condition
 *    - mutex: the associtated mutex locked while the condition isn't met
 * @info: see libpthread
 */
void _yrt_thread_cond_wait (_yrt_cond_t * cond, _yrt_mutex_t* lock);

/**
 * Trigger a condition
 * @params:
 *    - cond: the condition to trigger
 */
void _yrt_thread_cond_signal (_yrt_cond_t* cond);

/**
 * Broadcast a condition
 * @params:
 *    - cond: the condition to trigger
 */
void _yrt_thread_cond_broadcast (_yrt_cond_t* cond);

/**
 * Initialize a semaphore
 * @params:
 *    - pshared: the number of signal to emit before the semaphore is triggered
 *    - value: the value of the semaphore (see libpthread)
 * @returns:
 *    - sem: the semaphore
 */
void _yrt_thread_sem_init (sem_t * sem, int pshared, int value);

/**
 * Destroy a semaphore
 * @params:
 *    - sem: the semaphore to destroy
 */
void _yrt_thread_sem_destroy (sem_t * sem);

/**
 * Wait for a semaphore to be triggered
 * @params:
 *    - sem: the semaphore
 */
void _yrt_thread_sem_wait (sem_t * sem);

/**
 * @returns: the current counter of the semaphore
 * @params:
 *    - sem: the semaphore
 */
int32_t _yrt_thread_sem_get (sem_t * sem);

/**
 * Wait for a semaphore to be triggered or timeout
 * @params:
 *    - sem: the semaphore
 *    - sec: the timeout in second
 *    - nsec: the timeout in micro second
 */
uint8_t _yrt_thread_sem_wait_timeout (sem_t * sem, uint64_t sec, uint64_t nsec);

/**
 * Emit a new entry in the semaphore
 * @params:
 *    - sem: the semaphore to post
 * @info: if the number of post is equal to its value, the semaphore is triggered and wait-ers no longer wait
 */
void _yrt_thread_sem_post (sem_t * sem);


/**
 * Get the monitor of an object, allocate it if it the first time
 * @params:
 *   - object: a pointer to an object
 */
_yrt_mutex_t * _yrt_ensure_monitor (void* object);

/**
 * Lock a mutex in an atomic block
 * @params:
 *    - lock: the mutex to lock
 */
void _yrt_atomic_enter (_yrt_mutex_t * lock);

/**
 * Unlock a mutex in an atomic block
 * @params:
 *    - lock: the mutex to unlock
 */
void _yrt_atomic_exit (_yrt_mutex_t * lock);

/**
 * Lock a monitor in an atomic block
 * @params:
 *    - object: the object to lock
 */
void _yrt_atomic_monitor_enter (void * object);

/**
 * Unlock a monitor in an atomic block
 * @params:
 *    - lock: the mutex to unlock
 */
void _yrt_atomic_monitor_exit (void * lock);


#endif

#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <semaphore.h>

/**
 * Read data inside a Pipe
 * @params: 
 *    - stream: the pipe
 *    - size: the size to read (in bytes)
 * @return: the data read
 * @warning: this function is blocking until everything is read, or the pipe is closed
 */
void * _yrt_read_pipe (int stream, ulong size);

/**
 * Write data inside a Pipe
 * @params: 
 *    - stream: a pip
 *    - data: the data to write
 *    - size: the size (in bytes)
 */
void _yrt_write_pipe (int stream, void * data, ulong size);

/**
 * Create a new thread that will executed a function
 * @params: 
 *    - attr: the attribute of the thread (see libpthread )
 *    - call: the function to call
 *    - data: the parameters to pass to the function
 * @returns: 
 *    - id: the id of the created thread
 */
void _yrt_thread_create (pthread_t * id, pthread_attr_t* attr, void*(*call)(void*), void* data);

/**
 * Wait for the completion of a thread
 * @params: 
 *    - p: the id of the thread to wait
 * @returns: 
 *    - retval: the return value of the thread
 */
void _yrt_thread_join (pthread_t p, void** retval); 

/**
 * Detach a thread from the main thread, but does not kill it
 * @params:
 *    - p: the id of the thread
 */
void _yrt_thread_detach (pthread_t p);

/**
 * Kill a thread
 * @params: 
 *    - p: the id of the thread
 */
void _yrt_thread_cancel (pthread_t p);

/**
 * Exit the current thread and return a value
 * @params:
 *    - p: the value to return
 */
void _yrt_thread_exit (void* p);

/**
 * Create a new mutex
 * @params: 
 *    - data: the data of the mutex (see libpthread)
 * @returns: 
 *    - lock: the id of the lock
 */
void _yrt_thread_mutex_init (pthread_mutex_t* lock, pthread_mutexattr_t * data);

/**
 * Lock a mutex
 * @params: 
 *    - lock: the mutex to lock
 */
void _yrt_thread_mutex_lock (pthread_mutex_t* lock);

/**
 * Unlock a mutex 
 * @params: 
 *    - lock: the mutex to unlock
 */
void _yrt_thread_mutex_unlock (pthread_mutex_t* lock);

/**
 * Init a waitable conditional 
 * @params: 
 *    - data: the data of the condition (see libpthread)
 * @returns: 
 *    - cond: the condition 
 */
void _yrt_thread_cond_init (pthread_cond_t * cond, pthread_condattr_t* data);

/**
 * Wait for a condition to be triggered
 * @params: 
 *    - cond: the condition
 *    - mutex: the associtated mutex locked while the condition isn't met
 * @info: see libpthread
 */
void _yrt_thread_cond_wait (pthread_cond_t * cond, pthread_mutex_t* lock);

/**
 * Trigger a condition 
 * @params: 
 *    - cond: the condition to trigger
 */
void _yrt_thread_cond_signal (pthread_cond_t* cond);

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
 * Emit a new entry in the semaphore
 * @params:
 *    - sem: the semaphore to post
 * @info: if the number of post is equal to its value, the semaphore is triggered and wait-ers no longer wait
 */
void _yrt_thread_sem_post (sem_t * sem);

#endif

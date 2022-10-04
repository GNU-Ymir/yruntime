#include "thread.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
//#include <sys/wait.h>

#define GC_PTHREADS
#include <gc/gc.h>
#include <semaphore.h>

_yrt_mutex_t __monitor_mutex__ = PTHREAD_MUTEX_INITIALIZER;

void * _yrt_read_pipe (int stream, unsigned long long size) {
    void * z;
    void ** x = &z; 
    int r = read (stream, x, size);
    return *x;
}

void _yrt_write_pipe (int stream, void * data, unsigned long long size) {    
    int r = write (stream, &data, size);
}

void _yrt_thread_create (_yrt_thread_t * id, _yrt_attr_t* attr, void*(*call)(void*), void* data) {
    GC_pthread_create (id, attr, call, data);
}

void _yrt_thread_join (_yrt_thread_t p, void** retval) {
    GC_pthread_join (p, retval);
}

void _yrt_thread_detach (_yrt_thread_t p) {
    GC_pthread_detach (p);
}

#ifdef __linux__
void _yrt_thread_cancel (_yrt_thread_t p) {
    GC_pthread_cancel (p);
}

void _yrt_thread_exit (_yrt_thread_t p) {
    GC_pthread_exit (p);
}
#endif

void _yrt_thread_mutex_init (_yrt_mutex_t* lock, _yrt_mutexattr_t * data) {
    pthread_mutex_init (lock, data);
}

void _yrt_thread_mutex_lock (_yrt_mutex_t* lock) {
    pthread_mutex_lock (lock);
}

void _yrt_thread_mutex_unlock (_yrt_mutex_t* lock) {
    pthread_mutex_unlock (lock);
}

void _yrt_thread_cond_init (_yrt_cond_t * cond, _yrt_condattr_t* data) {
    pthread_cond_init (cond, data);
}

void _yrt_thread_cond_wait (_yrt_cond_t * cond, _yrt_mutex_t* lock) {
    pthread_cond_wait (cond, lock);
}

void _yrt_thread_cond_signal (_yrt_cond_t* cond) {
    pthread_cond_signal (cond);
}

void _yrt_thread_sem_init (sem_t * sem, int pshared, int value) {
    sem_init (sem, pshared, value);
}

void _yrt_thread_sem_destroy (sem_t * sem) {
    sem_destroy (sem);
}

void _yrt_thread_sem_wait (sem_t * sem) {
    sem_wait (sem);
}

void _yrt_thread_sem_post (sem_t * sem) {
    sem_post (sem);
}

_yrt_mutex_t * _yrt_ensure_monitor (void* object) {
    pthread_mutex_lock (&__monitor_mutex__);
    _yrt_mutex_t* mut = *((_yrt_mutex_t**) object + 1); // skip the vtable
    if (mut == NULL) {
	mut = GC_malloc (sizeof (_yrt_mutex_t));
	pthread_mutex_init (mut, NULL);
	*((_yrt_mutex_t**)object+1) = mut;
    }
    
    pthread_mutex_unlock (&__monitor_mutex__);
    return mut;    
}

void _yrt_atomic_enter (_yrt_mutex_t * lock) {
    pthread_mutex_lock (lock);
}

void _yrt_atomic_exit (_yrt_mutex_t * lock) {
    pthread_mutex_unlock (lock);
}

void _yrt_atomic_monitor_enter (void* object) {
    _yrt_mutex_t * lock = _yrt_ensure_monitor (object);
    pthread_mutex_lock (lock);
}

void _yrt_atomic_monitor_exit (void* object) {
    _yrt_mutex_t * lock = _yrt_ensure_monitor (object);
    pthread_mutex_unlock (lock);
}

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h>
#include <pthread.h>

#define GC_THREADS
#include <gc/gc.h>
#include <semaphore.h>

extern "C" void* _yrt_read_pipe (int stream, ulong size) {
    void * z;
    void ** x = &z; 
    read (stream, x, size);
    return *x;
}

extern "C" void _yrt_write_pipe (int stream, void * data, ulong size) {    
    write (stream, &data, size);
}

extern "C" void _yrt_thread_create (pthread_t * id, pthread_attr_t* attr, void*(*call)(void*), void* data) {
    GC_pthread_create (id, attr, call, data);
}

extern "C" void _yrt_thread_join (pthread_t p, void** retval) {
    GC_pthread_join (p, retval);
}

extern "C" void _yrt_thread_detach (pthread_t p) {
    GC_pthread_detach (p);
}

extern "C" void _yrt_thread_cancel (pthread_t p) {
    GC_pthread_cancel (p);
}

extern "C" void _yrt_thread_exit (void* p) {
    GC_pthread_exit (p);
}

extern "C" void _yrt_thread_mutex_init (pthread_mutex_t* lock, pthread_mutexattr_t * data) {
    pthread_mutex_init (lock, data);
}

extern "C" void _yrt_thread_mutex_lock (pthread_mutex_t* lock) {
    pthread_mutex_lock (lock);
}

extern "C" void _yrt_thread_mutex_unlock (pthread_mutex_t* lock) {
    pthread_mutex_unlock (lock);
}

extern "C" void _yrt_thread_cond_init (pthread_cond_t * cond, pthread_condattr_t* data) {
    pthread_cond_init (cond, data);
}

extern "C" void _yrt_thread_cond_wait (pthread_cond_t * cond, pthread_mutex_t* lock) {
    pthread_cond_wait (cond, lock);
}

extern "C" void _yrt_thread_cond_signal (pthread_cond_t* cond) {
    pthread_cond_signal (cond);
}

extern "C" void _yrt_thread_sem_init (sem_t * sem, int pshared, int value) {
    sem_init (sem, pshared, value);
}

extern "C" void _yrt_thread_sem_destroy (sem_t * sem) {
    sem_destroy (sem);
}

extern "C" void _yrt_thread_sem_wait (sem_t * sem) {
    sem_wait (sem);
}

extern "C" void _yrt_thread_sem_post (sem_t * sem) {
    sem_post (sem);
}

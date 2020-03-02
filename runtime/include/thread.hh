#pragma once
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

extern "C" void* _yrt_read_pipe (int stream, ulong size);

extern "C" void _yrt_write_pipe (int stream, void * data, ulong size);

extern "C" void _yrt_thread_create (pthread_t * id, pthread_attr_t* attr, void*(*call)(void*), void* data);

extern "C" void _yrt_thread_join (pthread_t p, void** retval); 

extern "C" void _yrt_thread_detach (pthread_t p);

extern "C" void _yrt_thread_cancel (pthread_t p);

extern "C" void _yrt_thread_exit (void* p);

extern "C" void _yrt_thread_mutex_init (pthread_mutex_t* lock, pthread_mutexattr_t * data);

extern "C" void _yrt_thread_mutex_lock (pthread_mutex_t* lock);

extern "C" void _yrt_thread_mutex_unlock (pthread_mutex_t* lock);

extern "C" void _yrt_thread_cond_init (pthread_cond_t * cond, pthread_condattr_t* data);

extern "C" void _yrt_thread_cond_wait (pthread_cond_t * cond, pthread_mutex_t* lock);

extern "C" void _yrt_thread_cond_signal (pthread_cond_t* cond);

extern "C" void _yrt_thread_sem_init (sem_t * sem, int pshared, int value);

extern "C" void _yrt_thread_sem_destroy (sem_t * sem);

extern "C" void _yrt_thread_sem_wait (sem_t * sem);

extern "C" void _yrt_thread_sem_post (sem_t * sem);

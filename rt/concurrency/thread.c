#include <rt/concurrency/thread.h>

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <sys/sysinfo.h>
#endif
#include <rt/utils/gc.h>


_yrt_mutex_t __monitor_mutex__ = PTHREAD_MUTEX_INITIALIZER;
_yrt_mutex_t __global_atom__ = PTHREAD_MUTEX_INITIALIZER;

#ifdef _WIN32
void * _read_pipe (void * stream, unsigned long long size) {
    void * z;
    void ** x = &z;
    ReadFile (stream, x, size, NULL, NULL);
    return *x;
}

void _write_pipe (void* stream, void * data, unsigned long long size) {
    WriteFile (stream, &data, size, NULL, NULL);
}
#else
void * _read_pipe (int stream, unsigned long long size) {
    void * z;
    void ** x = &z;
    int r = read (stream, x, size);
    return *x;
}

void _write_pipe (int stream, void * data, unsigned long long size) {
    int r = write (stream, &data, size);
}
#endif


int GC_pthread_create (_yrt_thread_t * id, _yrt_attr_t* attr, void*(*call)(void*), void* data);
int GC_pthread_join (_yrt_thread_t p, void** retval);
void GC_pthread_detach (_yrt_thread_t p);

/**
 * What a spawned thread needs to know to get going. It is left in the frame of the thread that
 * spawns it rather than allocated: 'data' is usually a GC pointer, and the collector scans the
 * spawning thread's stack -- it would not scan a malloc'ed struct, and a GC_malloc'ed one would
 * itself be unreachable during the hand-over. The spawning thread waits on 'copied' so that its
 * frame outlives the read.
 * */
typedef struct {
    void * (*call) (void*);
    void * data;
    sem_t copied;
} _yrt_thread_start_t;

static void _thread_remove_tls_roots (void * unused) {
    (void) unused;

    _gc_remove_tls_roots ();
}

static void * _thread_main (void * raw) {
    _yrt_thread_start_t * start = (_yrt_thread_start_t*) raw;
    void * (*call) (void*) = start-> call;
    void * data = start-> data;

    // '@thread' globals of this thread are unreachable for the collector until this call, so it
    // has to come before any Ymir code runs
    _gc_add_tls_roots ();

    // 'start' belongs to the spawning thread's frame and must not be touched from here on
    sem_post (&start-> copied);

    void * result = NULL;

    // a cleanup handler rather than a plain call after 'call', so that the roots also go away
    // when the thread is cancelled(see _thread_cancel) or exits early
    pthread_cleanup_push (&_thread_remove_tls_roots, NULL);
    result = call (data);
    pthread_cleanup_pop (1);

    return result;
}

void _thread_create (_yrt_thread_t * id, _yrt_attr_t* attr, void*(*call)(void*), void* data) {
    _yrt_thread_start_t start;
    start.call = call;
    start.data = data;
    sem_init (&start.copied, 0, 0);

    if (GC_pthread_create (id, attr, &_thread_main, &start) == 0) {
        sem_wait (&start.copied);
    }

    sem_destroy (&start.copied);
}

void _thread_join (_yrt_thread_t p, void** retval) {
    (void) GC_pthread_join (p, retval);
}

void _thread_detach (_yrt_thread_t p) {
    GC_pthread_detach (p);
}

#ifdef __linux__

void GC_pthread_cancel (_yrt_thread_t p);
void GC_pthread_exit (_yrt_thread_t p);

void _thread_cancel (_yrt_thread_t p) {
    GC_pthread_cancel (p);
}

void _thread_exit (_yrt_thread_t p) {
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

void _yrt_thread_barrier_init (_yrt_barrier_t * lock, uint32_t nb) {
    pthread_barrier_init (lock, NULL, nb);
}

void _yrt_thread_barrier_wait (_yrt_barrier_t * lock) {
    pthread_barrier_wait (lock);
}

void _yrt_thread_barrier_destroy (_yrt_barrier_t * lock) {
    pthread_barrier_destroy (lock);
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

void _yrt_thread_cond_broadcast (_yrt_cond_t* cond) {
    pthread_cond_broadcast (cond);
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

uint8_t _yrt_thread_sem_wait_timeout (sem_t * sem, uint64_t sec, uint64_t nsec) {
    struct timespec ts;
    if (clock_gettime (CLOCK_REALTIME, &ts) == -1) {
        return 0;
    }

    ts.tv_sec += sec;
    ts.tv_nsec += nsec;

    if (ts.tv_nsec > 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
    }

    do {
        int ret = sem_timedwait (sem, &ts);
        if (ret != 0) {
            if (errno != EINTR) { return 0; } // GC might trigger interruption signal
        } else {
            return 1;
        }
    } while (1);
}


uint8_t _yrt_thread_sem_wait_instant (sem_t * sem, uint64_t sec, uint64_t nsec) {
    struct timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = nsec;

    do {
        int ret = sem_timedwait (sem, &ts);
        if (ret != 0) {
            if (errno != EINTR) { return 0; } // GC might trigger interruption signal
        } else {
            return 1;
        }
    } while (1);
}

void _yrt_thread_sem_post (sem_t * sem) {
    sem_post (sem);
}

int32_t _yrt_thread_sem_get (sem_t * sem) {
    int32_t i;
    sem_getvalue (sem, &i);

    return i;
}

_yrt_mutex_t * _ensure_monitor (void* object) {
    pthread_mutex_lock (&__monitor_mutex__);
    _yrt_mutex_t* mut = *((_yrt_mutex_t**) object + 1); // skip the vtable
    if (mut == NULL) {
        mut = GC_malloc (sizeof (_yrt_mutex_t));

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init (mut, &attr);
        *((_yrt_mutex_t**)object+1) = mut;
    }
    
    pthread_mutex_unlock (&__monitor_mutex__);
    return mut;    
}

void _yrt_lock_global () {
    pthread_mutex_lock (&__global_atom__);
}

void _yrt_unlock_global () {
    pthread_mutex_unlock (&__global_atom__);
}

void _yrt_atomic_monitor_enter (void* object) {
    _yrt_mutex_t * lock = _ensure_monitor (object);
    pthread_mutex_lock (lock);
}

void _yrt_atomic_monitor_exit (void* object) {
    _yrt_mutex_t * lock = _ensure_monitor (object);
    pthread_mutex_unlock (lock);
}

uint32_t _yrt_get_nprocs () {
#ifdef __linux__
    return get_nprocs ();
#endif
#ifdef _WIN32 
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#endif
}

uint64_t _yrt_thread_self_id () {
    return pthread_self ();
}

// Boehm GC registers pthread_atfork handlers automatically, but that automatic protection is not
// reliable enough on its own (see the intermittent segfaults in CoverageTree::hitEnter racing
// SubProcess::start's fork() under concurrent TaskPool compilation, YMI-104): calling
// GC_atfork_prepare/parent/child explicitly around every fork() is the pattern the GC itself
// documents (gc.h) for programs that call fork() directly rather than through a GC-provided
// wrapper. This makes fork() quiesce all other threads (so none of them are mid-GC_malloc) before
// forking, and resume them right after.
uint32_t _yrt_fork () {
    GC_atfork_prepare ();
    pid_t pid = fork ();
    if (pid == 0) {
        GC_atfork_child ();
    } else {
        GC_atfork_parent ();
    }

    return (uint32_t) pid;
}


void _atomic_init () {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init (&__monitor_mutex__, &attr);
    pthread_mutex_init (&__global_atom__, &attr);
}

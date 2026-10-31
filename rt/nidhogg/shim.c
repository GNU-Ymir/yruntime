#ifndef _GNU_SOURCE
#  define _GNU_SOURCE // pthread_getattr_np, dl_iterate_phdr
#endif

#include <rt/nidhogg/shim.h>

#include <errno.h>
#include <link.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
// x86 uses TLS variant II: the static TLS block sits immediately below the thread pointer. Other
// targets lay TLS out differently and are left unhandled rather than guessed at.
#  define _YRT_NH_TLS 1
#endif

// SIGPWR is the one Boehm used, so nothing linked with the runtime expects to own it
#define _YRT_NH_SIGNAL SIGPWR

uint8_t _yrt_nh_marking = 0;
uint64_t _yrt_i_nh_globals = 0;

// the state of the calling thread in the collector, 0 until it is registered
static __thread uint64_t __NH_SELF__ = 0;

static pthread_key_t __NH_THREAD_KEY__;
static pthread_once_t __NH_THREAD_KEY_ONCE__ = PTHREAD_ONCE_INIT;

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          ATOMICS          =======================================
 * ====================================================================================================
 * ====================================================================================================
 */

uint8_t _yrt_i_nh_load8 (uint8_t * p) {
    return __atomic_load_n (p, __ATOMIC_ACQUIRE);
}

void _yrt_i_nh_store8 (uint8_t * p, uint8_t v) {
    __atomic_store_n (p, v, __ATOMIC_RELEASE);
}

uint8_t _yrt_i_nh_xchg8 (uint8_t * p, uint8_t v) {
    return __atomic_exchange_n (p, v, __ATOMIC_ACQ_REL);
}

uint64_t _yrt_i_nh_load64 (uint64_t * p) {
    return __atomic_load_n (p, __ATOMIC_ACQUIRE);
}

void _yrt_i_nh_store64 (uint64_t * p, uint64_t v) {
    __atomic_store_n (p, v, __ATOMIC_RELEASE);
}

uint64_t _yrt_i_nh_add64 (uint64_t * p, uint64_t v) {
    return __atomic_add_fetch (p, v, __ATOMIC_ACQ_REL);
}

uint64_t _yrt_i_nh_sub64 (uint64_t * p, uint64_t v) {
    return __atomic_sub_fetch (p, v, __ATOMIC_ACQ_REL);
}

uint8_t _yrt_i_nh_cas64 (uint64_t * p, uint64_t expected, uint64_t desired) {
    return __atomic_compare_exchange_n (p, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

void _yrt_i_nh_fence (void) {
    __atomic_thread_fence (__ATOMIC_SEQ_CST);
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ===================================          LOCKS          ========================================
 * ====================================================================================================
 * ====================================================================================================
 */

void * _yrt_i_nh_mutex_new (void) {
    pthread_mutex_t * m = malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (m, NULL);
    return m;
}

void _yrt_i_nh_mutex_lock (void * m) {
    pthread_mutex_lock ((pthread_mutex_t*) m);
}

void _yrt_i_nh_mutex_unlock (void * m) {
    pthread_mutex_unlock ((pthread_mutex_t*) m);
}

void * _yrt_i_nh_cond_new (void) {
    pthread_condattr_t attr;
    pthread_condattr_init (&attr);
    pthread_condattr_setclock (&attr, CLOCK_MONOTONIC);

    pthread_cond_t * c = malloc (sizeof (pthread_cond_t));
    pthread_cond_init (c, &attr);
    pthread_condattr_destroy (&attr);
    return c;
}

void _yrt_i_nh_cond_wait (void * c, void * m) {
    pthread_cond_wait ((pthread_cond_t*) c, (pthread_mutex_t*) m);
}

void _yrt_i_nh_cond_timedwait (void * c, void * m, uint64_t ns) {
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);

    uint64_t at = (uint64_t) ts.tv_nsec + ns;
    ts.tv_sec += (time_t) (at / 1000000000ULL);
    ts.tv_nsec = (long) (at % 1000000000ULL);
    pthread_cond_timedwait ((pthread_cond_t*) c, (pthread_mutex_t*) m, &ts);
}

void _yrt_i_nh_cond_broadcast (void * c) {
    pthread_cond_broadcast ((pthread_cond_t*) c);
}

void _yrt_i_nh_sleep_ns (uint64_t ns) {
    struct timespec ts = { .tv_sec = (time_t) (ns / 1000000000ULL), .tv_nsec = (long) (ns % 1000000000ULL) };
    while (nanosleep (&ts, &ts) != 0 && errno == EINTR) {}
}

uint64_t _yrt_i_nh_now_ns (void) {
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          THREADS          =======================================
 * ====================================================================================================
 * ====================================================================================================
 */

uint64_t _yrt_i_nh_get_self (void) {
    return __NH_SELF__;
}

void _yrt_i_nh_set_self (uint64_t state) {
    __NH_SELF__ = state;
}

void _yrt_i_nh_yield (void) {
    sched_yield ();
}

uint64_t _yrt_i_nh_thread_self (void) {
    return (uint64_t) pthread_self ();
}

// the highest address of the stack of the main thread, recorded by glibc at startup
extern void * __libc_stack_end;

void _yrt_i_nh_stack_bounds (uint64_t * lo, uint64_t * hi) {
    pthread_attr_t attr;
    void * addr = NULL;
    size_t size = 0;

    if (pthread_getattr_np (pthread_self (), &attr) == 0) {
        pthread_attr_getstack (&attr, &addr, &size);
        pthread_attr_destroy (&attr);
    }

    // the stack of the main thread is read from /proc/self/maps, which a sandbox may not mount
    if (size == 0 && getpid () == gettid ()) {
        addr = __builtin_frame_address (0);
        size = (char*) __libc_stack_end - (char*) addr;
    }

    if (size == 0) {
        static const char msg [] = "nidhogg: the bounds of the stack of a thread are unknown\n";
        _yrt_i_nh_fatal (msg, sizeof (msg) - 1);
    }

    *lo = (uint64_t) addr;
    *hi = (uint64_t) addr + (uint64_t) size;
}

#ifdef _YRT_NH_TLS

// The size of the PT_TLS segment of the main executable, rounded up to its alignment
static size_t __NH_TLS_SIZE__ = 0;
static pthread_once_t __NH_TLS_ONCE__ = PTHREAD_ONCE_INIT;

static int _read_tls_phdr (struct dl_phdr_info * info, size_t size, void * data) {
    (void) size;
    (void) data;

    // the main executable is the only object with an empty name, the TLS of a shared object is
    // allocated on demand by __tls_get_addr and is not part of the static block
    if (info-> dlpi_name != NULL && info-> dlpi_name [0] != '\0') return 0;
    for (int i = 0 ; i < info-> dlpi_phnum ; i++) {
        const ElfW (Phdr) * ph = &info-> dlpi_phdr [i];
        if (ph-> p_type == PT_TLS) {
            size_t align = ph-> p_align != 0 ? ph-> p_align : 1;
            __NH_TLS_SIZE__ = (ph-> p_memsz + align - 1) & ~(align - 1);
        }
    }

    return 1;
}

static void _read_tls_size (void) {
    dl_iterate_phdr (&_read_tls_phdr, NULL);
}

void _yrt_i_nh_tls_bounds (uint64_t * lo, uint64_t * hi) {
    pthread_once (&__NH_TLS_ONCE__, &_read_tls_size);

    char * tp = (char*) __builtin_thread_pointer ();
    *hi = (uint64_t) tp;
    *lo = (uint64_t) (tp - __NH_TLS_SIZE__);
}

#else

void _yrt_i_nh_tls_bounds (uint64_t * lo, uint64_t * hi) {
    *lo = 0;
    *hi = 0;
}

#endif

static void _thread_key_dtor (void * state) {
    _yrt_i_nh_thread_exit (state);
}

static void _create_thread_key (void) {
    pthread_key_create (&__NH_THREAD_KEY__, &_thread_key_dtor);
}

void _yrt_i_nh_set_thread_key (void * state) {
    pthread_once (&__NH_THREAD_KEY_ONCE__, &_create_thread_key);
    pthread_setspecific (__NH_THREAD_KEY__, state);
}

static void * _collector_main (void * data) {
    (void) data;
    _yrt_i_nh_collector_main ();
    return NULL;
}

static void * _finalizer_main (void * data) {
    (void) data;
    _yrt_i_nh_finalizer_main ();
    return NULL;
}

void _yrt_i_nh_spawn (uint8_t which) {
    sigset_t blocked, old;
    sigemptyset (&blocked);
    sigaddset (&blocked, _YRT_NH_SIGNAL);

    // the new thread inherits the mask: the collector is never a target of the handshakes, and
    // the finalizer unblocks the signal once it registered itself
    pthread_sigmask (SIG_BLOCK, &blocked, &old);

    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    pthread_create (&th, &attr, which == 0 ? &_collector_main : &_finalizer_main, NULL);
    pthread_attr_destroy (&attr);

    pthread_sigmask (SIG_SETMASK, &old, NULL);
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          HANDSHAKES          =====================================
 * ====================================================================================================
 * ====================================================================================================
 */

static void _handshake_handler (int sig, siginfo_t * info, void * uctx) {
    (void) sig;
    (void) info;
    (void) uctx;

    int saved = errno;

    // the registers of the interrupted code are in the signal frame, above this one on the stack
    volatile uint64_t marker = 0;
    _yrt_i_nh_handshake ((uint64_t) &marker);

    errno = saved;
}

void _yrt_i_nh_install_handshake (void) {
    struct sigaction sa;
    memset (&sa, 0, sizeof (sa));
    sa.sa_sigaction = &_handshake_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset (&sa.sa_mask);
    sigaction (_YRT_NH_SIGNAL, &sa, NULL);
}

void _yrt_i_nh_unblock_handshake (void) {
    sigset_t set;
    sigemptyset (&set);
    sigaddset (&set, _YRT_NH_SIGNAL);
    pthread_sigmask (SIG_UNBLOCK, &set, NULL);
}

int _yrt_i_nh_signal (uint64_t thread) {
    return pthread_kill ((pthread_t) thread, _YRT_NH_SIGNAL);
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ===================================          ROOTS          ========================================
 * ====================================================================================================
 * ====================================================================================================
 */

static int _read_data_phdr (struct dl_phdr_info * info, size_t size, void * data) {
    (void) size;
    (void) data;

    for (int i = 0 ; i < info-> dlpi_phnum ; i++) {
        const ElfW (Phdr) * ph = &info-> dlpi_phdr [i];
        if (ph-> p_type == PT_LOAD && (ph-> p_flags & PF_W)) {
            uint64_t lo = (uint64_t) (info-> dlpi_addr + ph-> p_vaddr);
            _yrt_i_nh_add_data_segment (lo, lo + (uint64_t) ph-> p_memsz);
        }
    }

    return 0;
}

void _yrt_i_nh_data_segments (void) {
    dl_iterate_phdr (&_read_data_phdr, NULL);
}

void _yrt_i_nh_call_finalizer (void * fn, void * obj) {
    ((void (*) (void*, void*)) fn) (obj, NULL);
}

void _yrt_i_nh_fatal (const char * msg, uint64_t len) {
    ssize_t r = write (2, msg, len);
    (void) r;
    abort ();
}

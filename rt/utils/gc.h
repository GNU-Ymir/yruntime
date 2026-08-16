#ifndef GC_H_
#define GC_H_

#include <sys/types.h>
#include <pthread.h>

#define GC_PTHREADS
#include <gc/gc.h>
#include <semaphore.h>

#include <gc/gc_disclaim.h>
#include <stdint.h>

void _yrt_disable_GC ();
void _yrt_enable_GC ();

uint8_t _yrt_is_GC_enabled ();

/**
 * Register the calling thread's static TLS block as a GC root.
 *
 * Boehm scans the stack and registers of every thread it knows about, plus the data segments,
 * but never thread local storage. A GC pointer whose only reference lives in a '@thread' global
 * is therefore invisible to the collector, and gets freed while the thread is still using it.
 * Registering the block makes '@thread' globals ordinary roots.
 *
 * Called by _yrt_init_runtime for the main thread, and by _yrt_thread_create for every thread
 * it spawns. A caller that registers a thread must unregister it before the thread dies: glibc
 * recycles a dead thread's TLS block for the next one, so a root left behind would both retain
 * garbage and be scanned after the block has been reused.
 * */
void _yrt_gc_add_tls_roots ();

/**
 * Unregister the calling thread's static TLS block, undoing _yrt_gc_add_tls_roots
 * */
void _yrt_gc_remove_tls_roots ();

#endif // GC_H_

#ifndef NIDHOGG_SHIM_H_
#define NIDHOGG_SHIM_H_

#include <stdint.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          NIDHOGG SHIM          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * The few pieces of the collector (core::nidhogg) that Ymir cannot express: the flag compiled
 * code tests before a write barrier, atomics, the locks of glibc, the threads of the collector,
 * the signal handler of the handshakes, and the walk of the loaded objects.
 * */

/**
 * Non zero while the collector marks: compiled code only calls the write barriers then
 * */
extern uint8_t _yrt_nh_marking;

/**
 * The address of the state of the collector, 0 until its first allocation
 * */
extern uint64_t _yrt_i_nh_globals;

/**
 * @returns: the address of the state of the calling thread in the collector, 0 if unregistered
 * */
uint64_t _yrt_i_nh_get_self (void);
void     _yrt_i_nh_set_self (uint64_t state);

uint8_t  _yrt_i_nh_load8 (uint8_t * p);
void     _yrt_i_nh_store8 (uint8_t * p, uint8_t v);

/**
 * @returns: the value '*p' had, '*p' being 'v' afterwards
 * */
uint8_t  _yrt_i_nh_xchg8 (uint8_t * p, uint8_t v);

uint64_t _yrt_i_nh_load64 (uint64_t * p);
void     _yrt_i_nh_store64 (uint64_t * p, uint64_t v);

/**
 * @returns: the value of '*p' after 'v' was added to it
 * */
uint64_t _yrt_i_nh_add64 (uint64_t * p, uint64_t v);

/**
 * @returns: the value of '*p' after 'v' was subtracted from it
 * */
uint64_t _yrt_i_nh_sub64 (uint64_t * p, uint64_t v);

/**
 * @returns: 1 iif '*p' was 'expected', and is 'desired' afterwards
 * */
uint8_t  _yrt_i_nh_cas64 (uint64_t * p, uint64_t expected, uint64_t desired);

void     _yrt_i_nh_fence (void);

/**
 * @returns: a new mutex, never freed
 * */
void *   _yrt_i_nh_mutex_new (void);
void     _yrt_i_nh_mutex_lock (void * m);
void     _yrt_i_nh_mutex_unlock (void * m);

/**
 * @returns: a new condition, never freed
 * */
void *   _yrt_i_nh_cond_new (void);
void     _yrt_i_nh_cond_wait (void * c, void * m);

/**
 * Wait on the condition, at most 'ns' nanoseconds
 * */
void     _yrt_i_nh_cond_timedwait (void * c, void * m, uint64_t ns);
void     _yrt_i_nh_cond_broadcast (void * c);

void     _yrt_i_nh_sleep_ns (uint64_t ns);
void     _yrt_i_nh_yield (void);

/**
 * @returns: a monotonic time in nanoseconds
 * */
uint64_t _yrt_i_nh_now_ns (void);

/**
 * @returns: the identifier of the calling thread, to pass to _yrt_i_nh_signal
 * */
uint64_t _yrt_i_nh_thread_self (void);

/**
 * Write the bounds of the stack of the calling thread
 * */
void     _yrt_i_nh_stack_bounds (uint64_t * lo, uint64_t * hi);

/**
 * Write the bounds of the static TLS block of the calling thread, empty where it is unknown
 * */
void     _yrt_i_nh_tls_bounds (uint64_t * lo, uint64_t * hi);

/**
 * Attach 'state' to the calling thread, _yrt_i_nh_thread_exit (state) being called when it ends
 * */
void     _yrt_i_nh_set_thread_key (void * state);

/**
 * Start a detached thread running _yrt_i_nh_collector_main (which == 0) or
 * _yrt_i_nh_finalizer_main (which == 1), the handshake signal blocked in it
 * */
void     _yrt_i_nh_spawn (uint8_t which);

/**
 * Install the handler of the handshake signal, which calls _yrt_i_nh_handshake
 * */
void     _yrt_i_nh_install_handshake (void);

/**
 * Unblock the handshake signal in the calling thread
 * */
void     _yrt_i_nh_unblock_handshake (void);

/**
 * Send the handshake signal to a thread
 * @returns: 0 on success
 * */
int      _yrt_i_nh_signal (uint64_t thread);

/**
 * Call _yrt_i_nh_add_data_segment for every writable segment of every loaded object
 * */
void     _yrt_i_nh_data_segments (void);

/**
 * Call the finalizer 'fn' of 'obj'
 * */
void     _yrt_i_nh_call_finalizer (void * fn, void * obj);

/**
 * Write a message on the standard error, and abort the process
 * */
void     _yrt_i_nh_fatal (const char * msg, uint64_t len);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          COLLECTOR          =====================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Implemented in Ymir by core::nidhogg, called by the shim
 * */
void _yrt_i_nh_handshake (uint64_t sp);
void _yrt_i_nh_collector_main (void);
void _yrt_i_nh_finalizer_main (void);
void _yrt_i_nh_thread_exit (void * state);
void _yrt_i_nh_add_data_segment (uint64_t lo, uint64_t hi);

/**
 * The entry points of core::nidhogg the runtime calls
 * */
void *   _yrt_nh_alloc (uint64_t size, uint8_t flags);
void     _yrt_nh_register_finalizer (void * obj, void * fn);
void     _yrt_nh_wb (void ** slot, void * val);
void     _yrt_nh_wb_range (void * dst, void * src, uint64_t size);
void     _yrt_nh_store (void * dst, void * src, uint64_t size);
void     _yrt_nh_shade_range (void * src, uint64_t size);
void     _yrt_nh_add_roots (void * lo, void * hi);
void     _yrt_nh_remove_roots (void * lo, void * hi);
void     _yrt_nh_register_thread (void);
void     _yrt_nh_collect (void);
void     _yrt_nh_wait_finalizers (void);
void     _yrt_nh_exit (void);
void     _yrt_nh_disable (void);
void     _yrt_nh_enable (void);
uint8_t  _yrt_nh_is_enabled (void);
void     _yrt_nh_atfork_prepare (void);
void     _yrt_nh_atfork_parent (void);
void     _yrt_nh_atfork_child (void);

/**
 * The flag of an allocation holding no pointer, which the collector never scans
 * */
#define NH_NOSCAN 1

#endif // NIDHOGG_SHIM_H_

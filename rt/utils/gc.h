#ifndef GC_H_
#define GC_H_

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>

#include <rt/nidhogg/shim.h>

void _yrt_disable_GC ();
void _yrt_enable_GC ();

uint8_t _yrt_is_GC_enabled ();

/**
 * Run a whole cycle of the collector, and wait until it is over
 * */
void _yrt_gc_collect ();

/**
 * Wait until the finalizers the collector queued have run
 * */
void _yrt_gc_wait_finalizers ();

/**
 * @returns: true while the collector marks, read anew at each call so that it is never hoisted out
 * of a loop of stores
 * */
static inline uint8_t _yrt_i_gc_marking (void) {
    return __atomic_load_n (&_yrt_nh_marking, __ATOMIC_RELAXED);
}

/**
 * @returns: 'size' bytes of zeroed memory of the collector, that may hold pointers
 * */
static inline void * _yrt_i_gc_alloc (uint64_t size) {
    return _yrt_nh_alloc (size, 0);
}

/**
 * @returns: 'size' bytes of zeroed memory of the collector, holding no pointer: it is never scanned
 * */
static inline void * _yrt_i_gc_alloc_atomic (uint64_t size) {
    return _yrt_nh_alloc (size, NH_NOSCAN);
}

/**
 * Store the 'size' bytes at 'src' into the memory of the collector at 'dst', through the write
 * barrier
 * */
static inline void _yrt_i_gc_store (void * dst, const void * src, uint64_t size) {
    if (_yrt_i_gc_marking ()) _yrt_nh_wb_range (dst, (void*) src, size);
    memmove (dst, src, size);
}

/**
 * Store the pointer 'val' into the slot 'slot' of the memory of the collector, through the write
 * barrier
 * */
static inline void _yrt_i_gc_store_ptr (void ** slot, void * val) {
    if (_yrt_i_gc_marking ()) _yrt_nh_wb (slot, val);
    *slot = val;
}

/**
 * Zero the 'size' bytes at 'dst' of the memory of the collector, through the write barrier
 * */
static inline void _yrt_i_gc_clear (void * dst, uint64_t size) {
    if (_yrt_i_gc_marking ()) _yrt_nh_wb_range (dst, NULL, size);
    memset (dst, 0, size);
}

/**
 * Copy the 'size' bytes at 'src' into the new memory of the collector at 'dst', shading what they
 * point to as the new memory is never scanned while the collector marks
 * */
static inline void _yrt_i_gc_copy_new (void * dst, const void * src, uint64_t size) {
    if (_yrt_i_gc_marking ()) _yrt_nh_shade_range ((void*) src, size);
    memcpy (dst, src, size);
}

#endif // GC_H_

#include "ymemory.h"
#include <memory.h>
#include "type.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* #ifndef GC_THREADS */
/* #define GC_THREADS */
/* #include <gc/gc.h> */
/* #include <gc/gc_disclaim.h> */
/* #endif */
#include "gc.h"
#include "thread.h"

pthread_mutex_t _yrt_dcopy_map_mutex;
struct _yrt_dcopy_map_node _yrt_dcopy_head = {.len = 0, .used = 0, .from = NULL, .to = NULL};

uint8_t* _yrt_dup_slice (uint8_t * addr, uint64_t len, uint64_t size) {
    uint8_t* x = (uint8_t*) GC_malloc (len * size);
    memcpy (x, addr, len * size);

    return x;
}


uint8_t* _yrt_alloc_array (uint8_t * addr, uint64_t len, uint64_t size) {
    uint8_t* x = (uint8_t*) GC_malloc (len * size);

#define NB 1024
    if (len <= NB || size > 1024) {
        for (uint64_t i = 0 ; i < len ; i++) {
            memcpy (x + (i * size), addr, size);
        }
    } else {
        uint8_t * copyBuf = malloc (NB * size);
        for (uint64_t i = 0 ; i < NB ; i++) {
            memcpy (copyBuf + (i * size), addr, size);
        }

        uint64_t rest = len % NB;
        uint64_t aligned = len - rest;

        for (uint64_t i = 0 ; i < aligned ; i += NB) {
            memcpy (x + (i * size), copyBuf, size * NB);
        }

        if (rest != 0) {
            memcpy (x + (aligned * size), copyBuf, rest * size);
        }

        free (copyBuf);
    }
#undef NB
    return x;
}


uint8_t* _yrt_dup_tuple (uint8_t* tu, uint64_t size) {
    char * x = (char*) GC_malloc (size);
    memcpy (x, tu, size);

    return x;
}

uint8_t* _yrt_new_block (uint64_t size, uint64_t len) {
    char * x = (char*) GC_malloc (len * size);
    return x;
}

_yrt_array_ _yrt_new_array (uint64_t size, uint64_t len) {
    char * x = (char*) GC_malloc (len * size);
    _yrt_array_ ret;
    ret.len = len;
    ret.data = x;
    return ret;
}

uint8_t* _yrt_dupl_any (uint8_t* data, uint64_t len) {
    char * x = GC_malloc (len);
    memcpy (x, data, len);
    return x;
}

void _yrt_destruct_class (GC_PTR obj, GC_PTR x) {
    void* vtable = *((void**)obj); // vtable
    // the second element is the destructor
    void(*__dtor) () = *(void(**)(void*)) ((void**) vtable + 1);
    (*__dtor) (obj);
}

void* _yrt_alloc_class (void* vtable) {
    // the first element stored in the vtable is the typeinfo
    _ytype_info *ti = *((_ytype_info**) vtable);

    // the second element is the destructor
    void(*__dtor) () = *(void(**)()) ((void**) vtable + 1);

    void* cl = GC_MALLOC (ti-> size);    
    *((void**)cl) = vtable; // vtable
    *((void**)cl+1) = NULL; // monitor

    // No need to finalize classes without destructors
    if ((*__dtor) != NULL) {
        GC_register_finalizer (cl, _yrt_destruct_class, NULL, NULL, NULL);
    }

    return cl;
}

uint8_t _yrt_cmp_class_of_type (void * to, void * from) {
    if (to == from) return 1;

    _ytype_info *tiTo = *((_ytype_info**) to);
    _ytype_info *tiFrom = *((_ytype_info**) from);

    while (tiFrom-> inner.len != 0) {
        _ytype_info * ancestor = ((_ytype_info*) (tiFrom-> inner.data));
        if (ancestor == tiTo) return 1;
        tiFrom = ancestor;
    }

    return 0;
}

_yrt_array_ _yrt_concat_slices (_yrt_array_ left, _yrt_array_ right, uint64_t size) {
    char* x = (char*) GC_malloc ((left.len + right.len) * size);
    memcpy (x, left.data, left.len * size);
    memcpy (x + (left.len * size), right.data, right.len * size);
    _yrt_array_ ret;
    ret.data = x;
    ret.len = left.len + right.len;
    return ret;
}

char _yrt_dcopy_map_is_started () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    char ret = (_yrt_dcopy_head.len != 0);
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
    return ret;
}

void _yrt_purge_dcopy_map () {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    _yrt_dcopy_head.len = 0;
    _yrt_dcopy_head.used = 0;
    _yrt_dcopy_head.from = NULL;
    _yrt_dcopy_head.to = NULL;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void _yrt_insert_dcopy_map (void * data, void * to) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    uint64_t index = _yrt_dcopy_head.used;
    if (index >= _yrt_dcopy_head.len) _yrt_dcopy_map_grow ();

    _yrt_dcopy_head.from [index] = data;
    _yrt_dcopy_head.to [index] = to;
    _yrt_dcopy_head.used += 1;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void* _yrt_find_dcopy_map (void * data) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    for (uint64_t i = 0 ; i < _yrt_dcopy_head.used ; i++) {
        if (_yrt_dcopy_head.from [i] == data) {
            _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
            return _yrt_dcopy_head.to [i];
        }
    }
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
    
    return NULL;
}


void _yrt_dcopy_map_grow () {
    uint64_t len = _yrt_dcopy_head.len;
    if (len == 0) len = 10;
    else len = len * 2;

    void** from_res = (void**) GC_malloc (len * sizeof (void*));
    void** to_res = (void**) GC_malloc (len * sizeof (void*));
    
    for (uint64_t i = 0 ; i < _yrt_dcopy_head.len ; i++) {
        from_res [i] = _yrt_dcopy_head.from [i];
        to_res [i] = _yrt_dcopy_head.to [i];
    }

    _yrt_dcopy_head.len = len;    
    _yrt_dcopy_head.from = from_res;
    _yrt_dcopy_head.to = to_res;
}




#include "ymemory.h"
#include <memory.h>
#include "type.h"
#include <stdio.h>

/* #ifndef GC_THREADS */
/* #define GC_THREADS */
/* #include <gc/gc.h> */
/* #include <gc/gc_disclaim.h> */
/* #endif */
#include "gc.h"
#include "thread.h"

pthread_mutex_t _yrt_dcopy_map_mutex;
struct _yrt_dcopy_map_node _yrt_dcopy_head = {.len = 0, .used = 0, .from = NULL, .to = NULL};

_yrt_array_ _yrt_dup_slice (_yrt_array_ arr, unsigned long size) {
    char* x = (char*) GC_malloc (arr.len * size);
    memcpy (x, arr.data, arr.len * size);
    _yrt_array_ ret;
    ret.len = arr.len;
    ret.data = x;
    
    return ret;
}

_yrt_array_ _yrt_alloc_array (void* data, unsigned long size, unsigned long len) {
    char* x = (char*) GC_malloc (len * size);
    for (unsigned long i = 0 ; i < len ; i++) {
	memcpy (x + (i*size), data, size);
    }

    _yrt_array_ ret;
    ret.len = len;
    ret.data = x;
    
    return ret;
}

void* _yrt_new_block (unsigned long size, unsigned long len) {
    char * x = (char*) GC_malloc (len * size);
    return x;
}

_yrt_array_ _yrt_new_array (unsigned long size, unsigned long len) {
    char * x = (char*) GC_malloc (len * size);
    _yrt_array_ ret;
    ret.len = len;
    ret.data = x;
    return ret;
}

void* _yrt_dupl_any (void* data, unsigned long len) {
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


_yrt_array_ _yrt_concat_slices (_yrt_array_ left, _yrt_array_ right, unsigned long size) {
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
    unsigned long index = _yrt_dcopy_head.used;
    if (index >= _yrt_dcopy_head.len) _yrt_dcopy_map_grow ();

    _yrt_dcopy_head.from [index] = data;
    _yrt_dcopy_head.to [index] = to;
    _yrt_dcopy_head.used += 1;
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
}

void* _yrt_find_dcopy_map (void * data) {
    _yrt_thread_mutex_lock (&_yrt_dcopy_map_mutex);
    for (unsigned long i = 0 ; i < _yrt_dcopy_head.used ; i++) {
	if (_yrt_dcopy_head.from [i] == data) {
	    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
	    return _yrt_dcopy_head.to [i];
	}
    }
    _yrt_thread_mutex_unlock (&_yrt_dcopy_map_mutex);
    
    return NULL;
}


void _yrt_dcopy_map_grow () {
    unsigned long len = _yrt_dcopy_head.len;
    if (len == 0) len = 10;
    else len = len * 2;

    void** from_res = (void**) GC_malloc (len * sizeof (void*));
    void** to_res = (void**) GC_malloc (len * sizeof (void*));
    
    for (unsigned long i = 0 ; i < _yrt_dcopy_head.len ; i++) {
	from_res [i] = _yrt_dcopy_head.from [i];
	to_res [i] = _yrt_dcopy_head.to [i];
    }

    _yrt_dcopy_head.len = len;    
    _yrt_dcopy_head.from = from_res;
    _yrt_dcopy_head.to = to_res;
}




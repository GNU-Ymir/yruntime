#include "../include/ymemory.h"
#include <memory.h>
#include "../include/type.h"
#include <stdio.h>

/* #ifndef GC_THREADS */
/* #define GC_THREADS */
/* #include <gc/gc.h> */
/* #include <gc/gc_disclaim.h> */
/* #endif */
#include "gc.h"

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

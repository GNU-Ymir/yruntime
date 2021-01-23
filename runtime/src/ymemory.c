#include "../include/ymemory.h"
#include <memory.h>

#ifndef GC_THREADS
#define GC_THREADS
#include <gc/gc.h>
#endif

_yarray _yrt_dup_slice (_yarray arr, unsigned long size) {
    char* x = (char*) GC_malloc (arr.len * size);
    memcpy (x, arr.data, arr.len * size);
    _yarray ret;
    ret.len = arr.len;
    ret.data = x;
    
    return ret;
}

_yarray _yrt_alloc_array (void* data, unsigned long size, unsigned long len) {
    char* x = (char*) GC_malloc (len * size);
    for (unsigned long i = 0 ; i < len ; i++) {
	memcpy (x + (i*size), data, size);
    }

    _yarray ret;
    ret.len = len;
    ret.data = x;
    
    return ret;
}

void* _yrt_new_block (unsigned long size, unsigned long len) {
    char * x = (char*) GC_malloc (len * size);
    return x;
}

_yarray _yrt_new_array (unsigned long size, unsigned long len) {
    char * x = (char*) GC_malloc (len * size);
    _yarray ret;
    ret.len = len;
    ret.data = x;
    return ret;
}

void* _yrt_dupl_any (void* data, unsigned long len) {
    char * x = GC_malloc (len);
    memcpy (x, data, len);
    return x;
}

void* _yrt_alloc_class (unsigned long len) {
    return GC_malloc (len);
}

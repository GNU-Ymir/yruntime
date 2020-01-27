#include <array.hh>
#include <memory.h>
#include <gc/gc.h>

extern "C" Array _yrt_dup_slice (Array arr, unsigned long size) {
    auto x = (char*) GC_malloc (arr.len * size);
    memcpy (x, arr.data, arr.len * size);
    return Array {arr.len, x};
}

extern "C" Array _yrt_alloc_array (void* data, unsigned long size, unsigned long len) {
    auto x = (char*) GC_malloc (len * size);
    for (unsigned long i = 0 ; i < len ; i++) {
	memcpy (x + (i*size), data, size);
    }
    return Array {len, x};
}

extern "C" Array _yrt_new_array (unsigned long size, unsigned long len) {
    auto x = (char*) GC_malloc (len * size);
    memset (x, 0, len * size);
    return Array {len, x};
}

extern "C" void* _yrt_dupl_any (void* data, unsigned long len) {
    auto x = GC_malloc (len);
    memcpy (x, data, len);
    return x;
}

extern "C" void* _yrt_alloc_class (unsigned long len) {
    return GC_malloc (len);
}

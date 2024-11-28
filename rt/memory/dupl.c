#include <rt/memory/dupl.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <string.h>

void _yrt_dup_slice (_yrt_slice_t * result, _yrt_slice_t * old, uint64_t size) {
	_yrt_alloc_slice_no_set (result, old-> len, size);
	memcpy (result-> data, old-> data, old-> len * size);
}

void _yrt_dup_tuple (uint8_t ** result, uint8_t * tu, uint64_t size) {
    *result = (char*) GC_malloc (size);
    memcpy (*result, tu, size);
}

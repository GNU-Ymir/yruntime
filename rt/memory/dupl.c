#include <rt/memory/dupl.h>
#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <string.h>

void _yrt_dup_slice (_yrt_slice_t * result, _yrt_slice_t * old, uint64_t size) {
	// filled aside: 'result' may be 'old' itself
	_yrt_slice_t res;
	_yrt_alloc_slice_no_set (&res, old-> len, size);
	_yrt_i_gc_copy_new (res.data, old-> data, old-> len * size);
	_yrt_i_gc_store (result, &res, sizeof (_yrt_slice_t));
}

void _yrt_dup_tuple (uint8_t ** result, uint8_t * tu, uint64_t size) {
    uint8_t * res = (uint8_t*) _yrt_i_gc_alloc (size);
    _yrt_i_gc_copy_new (res, tu, size);
    _yrt_i_gc_store_ptr ((void**) result, res);
}

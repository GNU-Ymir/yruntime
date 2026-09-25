#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


uint64_t _yrt_i_next_pow2 (uint64_t x) {
	if (x == 1) return 1;
	else {
		return 1 << (64 - __builtin_clzl (x - 1));
	}
}

uint8_t* _yrt_alloc_block(uint64_t size) {
    return (uint8_t*) GC_malloc(size);
}

void _yrt_alloc_slice_no_set (_yrt_slice_t * result, uint64_t len, uint64_t size) {
	if (len == 0) {
		memset (result, 0, sizeof (_yrt_slice_t));
		return;
	}

	size_t allocLen = _yrt_i_next_pow2 (len);
	size_t allocSize = allocLen * size;
	uint8_t* x = (uint8_t*) GC_malloc (allocSize + sizeof (_yrt_slice_blk_info_t));
	_yrt_slice_blk_info_t * blk = (_yrt_slice_blk_info_t*) (x);

	blk-> cap = allocLen;
	blk-> len = len;

	result-> len = len;
	result-> data = x + sizeof (_yrt_slice_blk_info_t);
	result-> blk_info = blk;
}

void _yrt_alloc_slice (_yrt_slice_t * result, uint8_t * addr, uint64_t len, uint64_t size) {
	_yrt_alloc_slice_no_set (result, len, size);
	uint8_t * x = result-> data;

    if (len <= 1024 || size > 1024) {
        for (uint64_t i = 0 ; i < len ; i++) {
            memcpy (x + (i * size), addr, size);
        }
    } else {
        uint8_t * copyBuf = malloc (1024 * size);
        for (uint64_t i = 0 ; i < 1024 ; i++) {
            memcpy (copyBuf + (i * size), addr, size);
        }

        uint64_t rest = len % 1024;
        uint64_t aligned = len - rest;

        for (uint64_t i = 0 ; i < aligned ; i += 1024) {
            memcpy (x + (i * size), copyBuf, size * 1024);
        }

        if (rest != 0) {
            memcpy (x + (aligned * size), copyBuf, rest * size);
        }

        free (copyBuf);
    }
}

void _yrt_concat_slices (_yrt_slice_t * result, _yrt_slice_t * left, _yrt_slice_t * right, uint64_t size) {
	_yrt_alloc_slice_no_set (result, left-> len + right-> len, size);
	memcpy (result-> data, left-> data, left-> len * size);
	memcpy (result-> data + (left-> len * size), right-> data, right-> len * size);
}

void _yrt_concat_slices_n (_yrt_slice_t * result, _yrt_slice_t * parts, uint64_t nb, uint64_t size) {
	uint64_t len = 0;
	for (uint64_t i = 0 ; i < nb ; i++) {
		len += parts [i].len;
	}

	// filled aside: 'result' may be one of the parts
	_yrt_slice_t res;
	_yrt_alloc_slice_no_set (&res, len, size);

	uint8_t * out = (uint8_t*) res.data;
	for (uint64_t i = 0 ; i < nb ; i++) {
		if (parts [i].len == 0) continue;
		memcpy (out, parts [i].data, parts [i].len * size);
		out += parts [i].len * size;
	}

	*result = res;
}

void _yrt_append_slice (_yrt_slice_t * result, _yrt_slice_t * right, uint64_t size) {
	// read before growing: 'right' may be 'result' itself
	uint64_t rlen = right-> len;
	void * rdata = right-> data;

	uint8_t * out = _yrt_i_grow_slice (result, rlen, size);
	memcpy (out, rdata, rlen * size);
}

uint8_t* _yrt_i_grow_slice (_yrt_slice_t * result, uint64_t len, uint64_t size) {
	uint64_t oldLen = result-> len;
	uint8_t * tail = (uint8_t*) result-> data + (oldLen * size);

	if (len == 0) return tail;

	uint8_t inPlace = (result-> blk_info != NULL);
	if (inPlace) { // the slice must end where the block does, and the block must have room left
		uint8_t * end = ((uint8_t*) result-> blk_info) + sizeof (_yrt_slice_blk_info_t) + (result-> blk_info-> len * size);
		inPlace = (end == tail) && (result-> blk_info-> len + len <= result-> blk_info-> cap);
	}

	if (!inPlace) {
		void * oldData = result-> data;
		_yrt_alloc_slice_no_set (result, oldLen + len, size);
		memcpy (result-> data, oldData, oldLen * size);

		return (uint8_t*) result-> data + (oldLen * size);
	}

	result-> blk_info-> len += len;
	result-> len += len;

	return tail;
}

void _yrt_grow_slice (_yrt_slice_t * result, _yrt_slice_t * appended, uint64_t len, uint64_t size) {
	uint8_t * out = _yrt_i_grow_slice (result, len, size);

	appended-> len = len;
	appended-> data = out;
	appended-> blk_info = result-> blk_info;
}

#include <rt/memory/alloc.h>

#include <rt/utils/gc.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


uint64_t _yrt_next_pow2 (uint64_t x) {
	if (x == 1) return 1;
	else {
		return 1 << (64 - __builtin_clzl (x - 1));
	}
}

void _yrt_alloc_slice_no_set (_yrt_slice_ * result, uint64_t len, uint64_t size) {
	if (len == 0) {
		memset (result, 0, sizeof (_yrt_slice_));
		return;
	}

	size_t allocLen = _yrt_next_pow2 (len);
	size_t allocSize = allocLen * size;
	uint8_t* x = (uint8_t*) GC_malloc (allocSize + sizeof (_yrt_slice_blk_info_));
	_yrt_slice_blk_info_ * blk = (_yrt_slice_blk_info_*) (x);

	blk-> cap = allocLen;
	blk-> len = len;

	result-> len = len;
	result-> data = x + sizeof (_yrt_slice_blk_info_);
	result-> blk_info = blk;
}

void _yrt_alloc_slice (_yrt_slice_ * result, uint8_t * addr, uint64_t len, uint64_t size) {
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

void _yrt_concat_slices (_yrt_slice_ * result, _yrt_slice_ * left, _yrt_slice_ * right, uint64_t size) {
	_yrt_alloc_slice_no_set (result, left-> len + right-> len, size);
	memcpy (result-> data, left-> data, left-> len * size);
	memcpy (result-> data + (left-> len * size), right-> data, right-> len * size);
}

void _yrt_append_slice (_yrt_slice_ * result, _yrt_slice_ * right, uint64_t size) {
	if (result-> blk_info == NULL) { // no blk info, slice may be allocated on the stack
		_yrt_slice_ tmp = *result;
		_yrt_concat_slices (result, &tmp, right, size);

		return;
	}

	uint8_t * end = ((uint8_t*) result-> blk_info) + (result-> blk_info-> len * size) + sizeof (_yrt_slice_blk_info_);
	if (end != result-> data + (result-> len * size)) { // blk info, but slice was cut down before the end
		_yrt_slice_ tmp = *result;
		_yrt_concat_slices (result, &tmp, right, size);

		return;
	}

	if (result-> blk_info-> len + right-> len > result-> blk_info-> cap) { // end of the slice but no space left
		_yrt_slice_ tmp = *result;
		_yrt_concat_slices (result, &tmp, right, size);

		return;
	}

	// enough space no need to make the slice grow
	uint64_t oldLen = result-> len;
	result-> blk_info-> len += right-> len;
	result-> len += right-> len;

	memcpy (result-> data + (oldLen * size), right-> data, right-> len * size);
}

#ifndef ALLOCS_H_
#define ALLOCS_H_

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          BASIC ALLOCS          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

#include <rt/memory/types.h>

/**
 * Allocate a new array and set the values of each element
 * @info: allocation made with the gc
 */
void _yrt_alloc_slice (_yrt_slice_ * result, uint8_t * addr, uint64_t len, uint64_t size);

/**
 * Allocate a new array and keep the memory as it is
 * @info: allocation made wit the gc
 *  */
void _yrt_alloc_slice_no_set (_yrt_slice_ * result, uint64_t len, uint64_t size);

/**
 * Concatenate two slices
 *  */
void _yrt_concat_slices (_yrt_slice_ * result, _yrt_slice_ * left, _yrt_slice_ * right, uint64_t size);

/**
 * Append elements at the end of a slice
 *  */
void _yrt_append_slice (_yrt_slice_ * result, _yrt_slice_ * right, uint64_t size);


#endif // ALLOCS_H_

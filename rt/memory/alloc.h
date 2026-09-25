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
 * Compute the new power of 2
 * @params:
 *   - x: an arbitrary number
 * @returns: the closest power of two to 'x'
 *  */
uint64_t _yrt_i_next_pow2 (uint64_t x);

/**
 * Allocate a new array and set the values of each element
 * @info: allocation made with the gc
 */
void _yrt_alloc_slice (_yrt_slice_t * result, uint8_t * addr, uint64_t len, uint64_t size);

/**
 * Allocate a new array and keep the memory as it is
 * @info: allocation made wit the gc
 *  */
void _yrt_alloc_slice_no_set (_yrt_slice_t * result, uint64_t len, uint64_t size);

/**
 * Allocate a new block without setting the data
 * @info: allocation made with the gc
 */
uint8_t* _yrt_alloc_block (uint64_t size);

/**
 * Concatenate two slices
 *  */
void _yrt_concat_slices (_yrt_slice_t * result, _yrt_slice_t * left, _yrt_slice_t * right, uint64_t size);

/**
 * Concatenate the 'nb' slices of 'parts', in order, into a single allocation
 *  */
void _yrt_concat_slices_n (_yrt_slice_t * result, _yrt_slice_t * parts, uint64_t nb, uint64_t size);

/**
 * Append elements at the end of a slice
 *  */
void _yrt_append_slice (_yrt_slice_t * result, _yrt_slice_t * right, uint64_t size);

/**
 * Extend a slice by 'len' uninitialized elements
 * @returns: the address of the first appended element
 * @info: reallocates with the gc when the block has no room left
 *  */
uint8_t* _yrt_i_grow_slice (_yrt_slice_t * result, uint64_t len, uint64_t size);

/**
 * Extend a slice by 'len' uninitialized elements, and view them
 * @params:
 *   - result: the slice to extend
 *   - appended: set to a view of the appended elements, for the caller to fill
 *  */
void _yrt_grow_slice (_yrt_slice_t * result, _yrt_slice_t * appended, uint64_t len, uint64_t size);


#endif // ALLOCS_H_

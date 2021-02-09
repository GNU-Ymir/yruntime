#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "yarray.h"

/**
 * Make a copy of the slice
 * @info: allocation made with the gc
 */
_yrt_array_ _yrt_dup_slice (_yrt_array_, unsigned long);

/**
 * Allocate a new array with the GC
 * @params: 
 *    - data: the data to put inside the allocated array (replicated at each index)
 *    - size: the size of each element in the array
 *    - len: the len of the array
 * @return: an allocated array with copied data inside it
 */
_yrt_array_ _yrt_alloc_array (void* data, unsigned long size, unsigned long len);

/**
 * Allocate a raw segment of memory with the GC
 * @params: 
 *    - size: the size of the inner type
 *    - len: the number of index (alloc size == len * size)
 * @return: the allocate block (GC)
 */
void* _yrt_new_block (unsigned long size, unsigned long len);

/**
 * Allocate a raw segment of memory with the GC
 * @params: 
 *    - size: the size of the inner type
 *    - len: the number of index (alloc size == len * size)
 * @return: the allocate block (GC) in a slice 
 */
_yrt_array_ _yrt_new_array (unsigned long size, unsigned long len);

/**
 * Make a copy of a raw segment of data into another raw segment of data allocated with GC
 * @params: 
 *    - data: the data to copy
 *    - len: the size of the data to copy
 * @return: the allocated block containing *data
 */
void* _yrt_dupl_any (void * data, unsigned long len);

/**
 * Allocate a segment a memory that will be used to store a class
 * @params: 
 *  - vtable: the vtable of the class to allocate
 * @return: an unset raw segment of data
 */
void* _yrt_alloc_class (void* vtable);

#endif

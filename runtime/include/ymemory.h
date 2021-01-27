#ifndef _MEMORY_H_
#define _MEMORY_H_

typedef struct {
    unsigned long len;
    void * data;
} _yarray;

/**
 * Make a copy of the slice
 * @info: allocation made with the gc
 */
_yarray _yrt_dup_slice (_yarray, unsigned long);

/**
 * Allocate a new array with the GC
 * @params: 
 *    - data: the data to put inside the allocated array (replicated at each index)
 *    - size: the size of each element in the array
 *    - len: the len of the array
 * @return: an allocated array with copied data inside it
 */
_yarray _yrt_alloc_array (void* data, unsigned long size, unsigned long len);

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
_yarray _yrt_new_array (unsigned long size, unsigned long len);

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
 *    - len: the size in bytes of the class
 * @return: an unset raw segment of data
 */
void* _yrt_alloc_class (unsigned long len);

#endif

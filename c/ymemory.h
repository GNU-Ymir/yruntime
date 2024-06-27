#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "yarray.h"

struct _yrt_dcopy_map_node {
    unsigned long len;
    unsigned long used;
    void** from;
    void** to;
};


struct _yrt_lazy_closure {
    void * closure;
    void (*func) (void* closure, void*ret);
};

struct _yrt_lazy_value {
    unsigned char set;
    void* data;
    uint32_t size;
    struct _yrt_lazy_closure closure;
};

/**
 * Make a copy of the slice
 * @info: allocation made with the gc
 */
uint8_t* _yrt_dup_slice (uint8_t * addr, uint64_t len, uint64_t size);


/**
 * Make a copy of the slice
 * @info: allocation made with the gc
 */
uint8_t* _yrt_alloc_array (uint8_t * addr, uint64_t len, uint64_t size);


/**
 * Allocate a raw segment of memory with the GC
 * @params: 
 *    - size: the size of the inner type
 *    - len: the number of index (alloc size == len * size)
 * @return: the allocate block (GC)
 */
uint8_t* _yrt_new_block (uint64_t size, uint64_t len);

/**
 * Allocate a raw segment of memory with the GC
 * @params: 
 *    - size: the size of the inner type
 *    - len: the number of index (alloc size == len * size)
 * @return: the allocate block (GC) in a slice 
 */
_yrt_array_ _yrt_new_array (uint64_t size, uint64_t len);

/**
 * Make a copy of a raw segment of data into another raw segment of data allocated with GC
 * @params: 
 *    - data: the data to copy
 *    - len: the size of the data to copy
 * @return: the allocated block containing *data
 */
uint8_t* _yrt_dupl_any (uint8_t* data, uint64_t len);

/**
 * Allocate a segment a memory that will be used to store a class
 * @params: 
 *  - vtable: the vtable of the class to allocate
 * @return: an unset raw segment of data
 */
void* _yrt_alloc_class (void* vtable);

/**
 * A dcopy map is started if it contains at least one element
 * @return: true if the deep copy map has been started, false otherwise
 */
char _yrt_dcopy_map_is_started ();

/**
 * Purge the map containing all the current deep copy information
 */
void _yrt_purge_dcopy_map ();


/**
 * Search in the map if the element has already been deep copied, if true, then simply returns the pointer to that element
 * @params: 
 *   - data: the pointer to an element 
 * @return: a pointer either null, or the an element
 */
void* _yrt_find_dcopy_map (void* data);


/**
 * Insert an element that has been deep copied in the map
 * @params:
 * - data: the old pointer
 * - value: the copied pointer
 */
void _yrt_insert_dcopy_map (void* data, void* value);


void _yrt_dcopy_map_grow ();

/**
 * Check if the lazy value is set, if not construct it
 */
void* _yrt_call_lazy (struct _yrt_lazy_value * value);

#endif

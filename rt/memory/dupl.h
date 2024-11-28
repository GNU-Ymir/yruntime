#ifndef DUPL_H_
#define DUPL_H_

#include <rt/memory/types.h>

/**
 * Make a copy of the slice
 * @info: allocation made with the gc
 */
void _yrt_dup_slice (_yrt_slice_t * result, _yrt_slice_t * old, uint64_t size);

/**
 * Duplicate a tuple
 *  */
void _yrt_dup_tuple (uint8_t ** result, uint8_t * old, uint64_t size);


#endif // DUPL_H_

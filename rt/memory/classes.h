#ifndef CLASSES_H_
#define CLASSES_H_

#include <rt/utils/gc.h>
#include <rt/memory/types.h>


/*!
 * ====================================================================================================
 * ====================================================================================================
 * ==================================          CLASS ALLOCS          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Allocate a segment a memory that will be used to store a class
 * @params:
 *  - vtable: the vtable of the class to allocate
 * @return: an unset raw segment of data
 */
void* _yrt_alloc_class (void* vtable);

/**
 * Allocate a class whose destructor is not registered yet: dropped before
 * _yrt_commit_class, it is collected without its destructor ever running
 * @params:
 *  - vtable: the vtable of the class to allocate
 * @return: a zeroed segment, but for its vtable
 */
void* _yrt_alloc_class_pending (void* vtable);

/**
 * Register the destructor of a class allocated by _yrt_alloc_class_pending
 * @params:
 *  - cl: the class, every field of which is written
 */
void _yrt_commit_class (void* cl);

/**
 * Called when a class is destroyed by the gc
 *  */
void _yrt_i_destruct_class (GC_PTR obj, GC_PTR x);

#endif // CLASSES_H_

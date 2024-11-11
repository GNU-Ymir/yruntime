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
 * Called when a class is destroyed by the gc
 *  */
void _yrt_destruct_class (GC_PTR obj, GC_PTR x);

#endif // CLASSES_H_

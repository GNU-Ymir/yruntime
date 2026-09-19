#ifndef LAZY_H_
#define LAZY_H_

#include <rt/memory/types.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ======================================          LAZY          ======================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Check if the lazy value is set, if not construct it
 * Thread-safe: one thread runs the initializer while the others wait for it,
 * an initializer that throws leaves the value unset for the next access, and one that
 * reaches its own lazy again (e.g. through a GC finalizer) runs a nested initialization
 */
void* _yrt_call_lazy (_yrt_lazy_value_t * value);


#endif // LAZY_H_

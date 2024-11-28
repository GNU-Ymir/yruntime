#ifndef TINFO_H_
#define TINFO_H_

#include <rt/memory/types.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ====================================          TYPEINFO          ====================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Verify the equallity between the type a and the type b
 *
 */
char _yrt_type_equals (_yrt_type_info a, _yrt_type_info b);

/**
 * \return the name of the typeinfo symbol of a class from the class name (mangled)
 */
_yrt_slice_t _yrt_type_typeinfo_name (_yrt_slice_t className);

/**
 * \return the name of the vtable symbol of a class from the class name
 */
_yrt_slice_t _yrt_type_vtable_name (_yrt_slice_t className);

/**
 * \return the name of the constructor with no parameters of the class (mangled)
 * \info, its just the name, there is no check wether there is really such a constructor
 */
_yrt_slice_t _yrt_type_constructor_no_param_name (_yrt_slice_t className);

/**
 * Unsafe cast that does nothing
 *  */
void* _yrt_unsafe_cast (void*);

#endif // TINFO_H_

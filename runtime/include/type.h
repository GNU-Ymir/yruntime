#ifndef _YTYPEINFO_H_
#define _YTYPEINFO_H_

#include "ymemory.h"
#include "ystring.h"
#include "yarray.h"

typedef struct {    
    unsigned int id;
    unsigned long size;
    _yrt_array_ inner;
    _yrt_array_ name;
}  _ytype_info;

enum _ytype_ids {
    ARRAY        = 1,
    BOOL         = 2,
    CHAR         = 3,
    CLOSURE      = 4,
    FLOAT        = 5,
    FUNC_PTR     = 6,
    SIGNED_INT   = 7,
    UNSIGNED_INT = 8,
    POINTER      = 9,
    SLICE        = 10,
    STRUCT       = 11,
    TUPLE        = 12,
    OBJECT       = 13,
    VOID         = 14,
};


/**
 * Verify the equallity between the type a and the type b 
 * 
 */
char _yrt_type_equals (_ytype_info a, _ytype_info b);

/**
 * \return the name of the typeinfo symbol of a class from the class name (mangled)
 */
_yrt_c8_array_ _yrt_type_typeinfo_name (_yrt_c8_array_ className);

/**
 * \return the name of the vtable symbol of a class from the class name 
 */
_yrt_c8_array_ _yrt_type_vtable_name (_yrt_c8_array_ className);

/**
 * \return the name of the constructor with no parameters of the class (mangled)
 * \info, its just the name, there is no check wether there is really such a constructor
 */
_yrt_c8_array_ _yrt_type_constructor_no_param_name (_yrt_c8_array_ className);

#endif

#ifndef _YTYPEINFO_H_
#define _YTYPEINFO_H_

#include "ymemory.h"

typedef struct {    
    unsigned int id;
    unsigned long size;
    _yarray inner;
    _yarray name;
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

#endif

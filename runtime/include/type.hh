#pragma once
#include <array.hh>

struct TypeInfo {    
    unsigned int id;
    unsigned long size;
    Array inner;
    Array name;
};

enum TypeIDs {
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


extern "C" bool _Y4core8typeinfo6equalsF4core8typeinfo8TypeInfo4core8typeinfo8TypeInfoZb (TypeInfo a, TypeInfo b);
extern "C" bool _yrt_type_equals (TypeInfo a, TypeInfo b);

#pragma once
#include <array.hh>

struct TypeInfo {    
    unsigned int id;
    unsigned long size;
    Array inner;
    Array name;
};


extern "C" bool _Y4core8typeinfo6equalsF4core8typeinfo8TypeInfo4core8typeinfo8TypeInfoZb (TypeInfo a, TypeInfo b);

#include <type.hh>
#include <stdio.h>

extern "C" bool _y_cmp_typeinfo (TypeInfo* left, TypeInfo* right) {
    return right-> vtable-> equals (right, *left);
}

extern "C" void* _y_dynamic_cast (TypeInfo *** elem, TypeInfo* right) {
    // elem is a p!class, *((elem[0] => vtable) [0] => typeinfo)
    if (right-> vtable-> equals (right, *(elem [0][0]))) {	
	return elem;
    } return 0;
}

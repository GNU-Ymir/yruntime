#include "../include/type.h"
#include "../include/array.h"
#include "../include/demangle.h"


char _yrt_type_equals (_ytype_info a, _ytype_info b) {
    if (a.id == b.id) {
	if (a.id == BOOL ||
            a.id == CHAR ||
            a.id == FLOAT ||
            a.id == SIGNED_INT ||
            a.id == UNSIGNED_INT) {
            return a.size == b.size;
	} else if (a.inner.len == b.inner.len && (a.id != STRUCT && a.id != OBJECT)) {
	    for (unsigned long i = 0 ; i < a.inner.len ; i++) {
		if (!_yrt_type_equals (((_ytype_info*) a.inner.data) [i], ((_ytype_info*) b.inner.data) [i]))
		    return 0;
	    }
	} else if (a.id == OBJECT) {
	    if (a.name.data != b.name.data) {
		if (a.inner.len != 0) {
		    return _yrt_type_equals (((_ytype_info*) a.inner.data) [0], b);
		} else return 0;
	    } else return 1;
	} else if (a.id == STRUCT) {
	    return a.name.data == b.name.data;
	}
    }
    return 0;    
}

void* _yrt_unsafe_cast (void* x) {
    return x;
}

_yrt_c8_array_ _yrt_type_typeinfo_name (_yrt_c8_array_ mangled) {
    _ystring name = str_fit (str_concat (str_create_len ("_Y", 2),
					 str_concat (str_create_len (mangled.data, mangled.len), str_create_len ("TI", 2))));
    
    _yrt_c8_array_ arr;
    arr.data = name.data;
    arr.len = name.len;
    return arr;
}


_yrt_c8_array_ _yrt_type_vtable_name (_yrt_c8_array_ mangled) {
    _ystring name = str_fit (str_concat (str_create_len ("_Y", 2),
					 str_concat (str_create_len (mangled.data, mangled.len), str_create_len ("VT", 2))));
    
    _yrt_c8_array_ arr;
    arr.data = name.data;
    arr.len = name.len;
    return arr;
}


_yrt_c8_array_ _yrt_type_constructor_no_param_name (_yrt_c8_array_ mangled) {
    _ystring name = str_concat (str_create_len ("_Y", 2),
				str_concat (str_create_len (mangled.data, mangled.len), str_create_len ("4selfF", 6)));
    
    name = str_concat (name, str_create_len ("xP", 2));
    name = str_concat (name, str_from_int (mangled.len + 1));
    name = str_concat (name, str_create_len ("x", 1));
    name = str_concat (name, str_create_len (mangled.data, mangled.len));
    name = str_concat (name, str_create_len ("ZxP", 3));
    name = str_concat (name, str_from_int (mangled.len + 1));
    name = str_concat (name, str_create_len ("x", 1));
    name = str_concat (name, str_create_len (mangled.data, mangled.len));
    name = str_fit (name);

    _yrt_c8_array_ arr;
    arr.data = name.data;
    arr.len = name.len;
    return arr;
}

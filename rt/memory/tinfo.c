#include <rt/memory/tinfo.h>

#include <rt/utils/string.h>
#include <rt/memory/alloc.h>

#include <stddef.h>

char _yrt_type_equals (_yrt_type_info a, _yrt_type_info b) {
    if (a.id == b.id) {
	if (a.id == BOOL_ ||
            a.id == CHAR_ ||
            a.id == FLOAT_ ||
            a.id == SIGNED_INT ||
            a.id == UNSIGNED_INT) {
            return a.size == b.size;
	} else if (a.inner.len == b.inner.len && (a.id != STRUCT && a.id != OBJECT)) {
	    for (unsigned long i = 0 ; i < a.inner.len ; i++) {
		if (!_yrt_type_equals (((_yrt_type_info*) a.inner.data) [i], ((_yrt_type_info*) b.inner.data) [i]))
		    return 0;
	    }
	    return 1;
	} else if (a.id == OBJECT) {
	    if (a.name.data != b.name.data) {
		if (a.inner.len != 0) {
		    return _yrt_type_equals (*((_yrt_type_info*) a.inner.data), b);
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

_yrt_slice_ _yrt_type_typeinfo_name (_yrt_slice_ mangled) {
    _yrt_slice_ name = str_copy_len ("_Y", 2);

	_yrt_slice_ tmp = str_create_len (mangled.data, mangled.len);
	_yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("TI", 2);
	_yrt_append_slice (&name, &tmp, 1);

    return name;
}


_yrt_slice_ _yrt_type_vtable_name (_yrt_slice_ mangled) {
    _yrt_slice_ name = str_copy_len ("_Y", 2);
	_yrt_slice_ tmp = str_create_len (mangled.data, mangled.len);

	_yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("VT", 2);
	_yrt_append_slice (&name, &tmp, 1);

    return name;
}


_yrt_slice_ _yrt_type_constructor_no_param_name (_yrt_slice_ mangled) {
    _yrt_slice_ name = str_copy_len ("_Y", 2);

	_yrt_slice_ tmp = str_create_len (mangled.data, mangled.len);
	_yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("4selfF", 6);
	_yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("xP", 2);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_from_int (mangled.len + 1);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("x", 1);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len (mangled.data, mangled.len);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("ZxP", 3);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_from_int (mangled.len + 1);
	_yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len ("x", 1);
    _yrt_append_slice (&name, &tmp, 1);

	tmp = str_create_len (mangled.data, mangled.len);
    _yrt_append_slice (&name, &tmp, 1);

    return name;
}

uint8_t _yrt_cmp_class_of_type (void * tiTo, void * from) {
    _yrt_type_info *tiFrom = *((_yrt_type_info**) from);
    if (tiTo == tiFrom) return 1;

    while (tiFrom-> inner.len != 0) {
        _yrt_type_info * ancestor = ((_yrt_type_info*) (tiFrom-> inner.data));
        if (ancestor == tiTo) return 1;
        tiFrom = ancestor;
    }

    return 0;
}

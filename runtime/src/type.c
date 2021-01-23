#include "../include/type.h"

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

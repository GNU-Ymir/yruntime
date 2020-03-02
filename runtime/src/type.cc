#include <type.hh>

extern "C" bool _yrt_type_equals (TypeInfo a, TypeInfo b) {
    if (a.id == b.id) {
	if (a.id == TypeIDs::BOOL ||
            a.id == TypeIDs::CHAR ||
            a.id == TypeIDs::FLOAT ||
            a.id == TypeIDs::SIGNED_INT ||
            a.id == TypeIDs::UNSIGNED_INT) {
            return a.size == b.size;
	} else if (a.inner.len == b.inner.len && (a.id != TypeIDs::STRUCT && a.id != TypeIDs::OBJECT)) {
	    for (unsigned long i = 0 ; i < a.inner.len ; i++) {
		if (!_yrt_type_equals (((TypeInfo*) a.inner.data) [i], ((TypeInfo*) b.inner.data) [i]))
		    return false;
	    }
	} else if (a.id == TypeIDs::OBJECT) {
	    if (a.name.data != b.name.data) {
		if (a.inner.len != 0) {
		    return _yrt_type_equals (((TypeInfo*) a.inner.data) [0], b);
		} else return false;
	    } else return true;
	} else if (a.id == TypeIDs::STRUCT) {
	    return a.name.data == b.name.data;
	}
    }
    return false;    
}

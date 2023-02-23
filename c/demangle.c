#include "demangle.h"
#include <stdio.h>


// _Y4core5array10OutOfArray4selfFxP24x4core5array10OutOfArrayZxP24x4core5array10OutOfArray0

int _yrt_demangle_number (char * data, int * current) {
    int nb = 0;
    while (*data >= '0' && *data <= '9') {
	*current += 1;
	nb = nb * 10 + (*data - '0');
	data ++;
    }
    return nb;
}
					  
_ystring _yrt_demangle_symbol (char * data, uint64_t len) {
    _ystring ret = str_empty ();
    if (len <= 2 || data [0] != '_' || data [1] != 'Y')
	return str_create_len (data, len);
    
    int current = 2;
    int i = 0;
    while (1) {
	int nb = _yrt_demangle_number (data + current, &current);
	if (nb != 0) {
	    if (i != 0) ret = str_concat_c_str (ret, "::");
	    ret = str_concat (ret, str_create_len (data + current, nb));
	    current += nb;
	    i += 1;
	} else break;	
    }

    if (data [current] == 'F') {
	ret = str_concat_c_str (ret, " (...)");
    } 
    
    return ret;    
}

_yrt_c8_array_ _yrt_demangle_symbol_to_slice (char * data, uint64_t len) {
    _ystring s = _yrt_demangle_symbol (data, len);
    _yrt_c8_array_ ret = {.len = s.len, .data = s.data};
    return ret;
}

_yrt_c8_array_ _yrt_mangle_path (_yrt_c8_array_ data) {
    _ystring str = str_empty ();
    int current = 0;
    int start = 0;
    int i = 0;
    while (i < data.len) {
	if (data.data [i] == ':') {
	    str = str_concat (str, str_from_int (current));
	    _ystring aux = str_create_len (data.data + start, current);
	    str = str_concat (str, aux);
	    start += current + 2; // skip ::
	    current = 0;
	    i += 1;
	} else if (data.data [i] != '\0') {
	    current += 1;
	}
	i += 1;
    }

    if (current != 0) {
	str = str_concat (str, str_from_int (current));
	_ystring aux = str_create_len (data.data + start, current);
	str = str_concat (str, aux);
    }
    
    str = str_fit (str);
    _yrt_c8_array_ arr;
    arr.len = str.len;
    arr.data = str.data;
    return arr;
}


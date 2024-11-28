#include <rt/utils/demangle.h>
#include <rt/memory/alloc.h>
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
					  
_yrt_slice_t _yrt_demangle_symbol (char * data, uint64_t len) {
    if (len <= 2 || data [0] != '_' || data [1] != 'Y') {
		return str_create_len (data, len);
	}

	_yrt_slice_t ret = str_empty ();
    int current = 2;
    int i = 0;
    while (1) {
		int nb = _yrt_demangle_number (data + current, &current);
		if (nb != 0) {
			if (i != 0) {
				_yrt_slice_t tmp = str_create_len ("::", len);
				_yrt_append_slice (&ret, &tmp, 1);
			}

			_yrt_slice_t tmp = str_create_len (data + current, nb);
			_yrt_append_slice (&ret, &tmp, 1);
			current += nb;
			i += 1;
		} else break;
    }

    if (data [current] == 'F') {
		_yrt_slice_t tmp = str_create_len (" (...)", 6);
		_yrt_append_slice (&ret, &tmp, 1);
    } 
    
    return ret;    
}

_yrt_slice_t _yrt_demangle_symbol_to_slice (char * data, uint64_t len) {
    return _yrt_demangle_symbol (data, len);
}

_yrt_slice_t _yrt_mangle_path (_yrt_slice_t data) {
    _yrt_slice_t str = str_empty ();
    int current = 0;
    int start = 0;
    int i = 0;
    while (i < data.len) {
		if (((uint8_t*) data.data) [i] == ':') {
			_yrt_slice_t tmp = str_from_int (current);
			_yrt_append_slice (&str, &tmp, 1);

			tmp = str_create_len (data.data + start, current);
			_yrt_append_slice (&str, &tmp, 1);
			start += current + 2; // skip ::
			current = 0;
			i += 1;
		} else if (((uint8_t*) data.data) [i] != '\0') {
			current += 1;
		}

		i += 1;
    }

    if (current != 0) {
		_yrt_slice_t tmp = str_from_int (current);
		_yrt_append_slice (&str, &tmp, 1);

		tmp = str_create_len (data.data + start, current);
		_yrt_append_slice (&str, &tmp, 1);
    }

	return str;
}

#include "../include/demangle.h"
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
					  
_ystring _yrt_demangle_symbol (char * data, unsigned long len) {
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

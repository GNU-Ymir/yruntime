#include "ystring.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gc.h"

_ystring str_grow (_ystring self, unsigned long min) {
    unsigned long n_len = self.len;
    if (n_len == 0) n_len = 100;
    else n_len = n_len * 2;
    
    if (n_len < min) n_len = min;

    char* res = (char*) GC_malloc (n_len);
    memcpy (res, self.data, self.len);

    unsigned long old_len = self.len;

    _ystring ret;
    ret.data = res;
    ret.len = old_len;
    ret.capacity = n_len;
    
    return ret;
}


_ystring str_concat (_ystring left, _ystring right) {
    unsigned long min = (left.len + right.len) + 1;
    if (left.capacity < min)
	left = str_grow (left, min);

    memcpy (left.data + left.len, right.data, right.len);
    left.data [min] = '\0';
    
    left.len = left.len + right.len;
    
    return left;
}

_ystring str_concat_c_str (_ystring left, const char * data) {
    return str_concat (left, str_create (data));
}

_ystring str_from_int (int value) {
    unsigned int len = snprintf (NULL, 0, "%d", value);
    _ystring ret;
    ret.capacity = 0;
    ret.len = 0;
    ret = str_grow (ret, len);
       
    sprintf (ret.data, "%d", value);
    ret.len = len;
    return ret;
}

_ystring str_from_char (char value) {
    unsigned int len = snprintf (NULL, 0, "%c", value);
    _ystring ret;
    ret.capacity = 0;
    ret.len = 0;
    ret = str_grow (ret, len);
       
    sprintf (ret.data, "%c", value);
    ret.len = len;
    return ret;
}

_ystring str_from_ptr (void* value) {
    unsigned int len = snprintf (NULL, 0, "%p", value);
    _ystring ret;
    ret.capacity = 0;
    ret.len = 0;
    ret = str_grow (ret, len);
       
    sprintf (ret.data, "%p", value);
    ret.len = len;
    return ret;
}

_ystring str_create (const char * data) {
    _ystring ret;
    ret.data = (char*) data;
    if (ret.data != NULL) {
	ret.len = strlen (data);
	ret.capacity = ret.len;
    } else {
	ret.capacity = ret.len = 0;
    }
	 
    return ret;
}

_ystring str_create_len (const char * data, unsigned long len) {
    _ystring ret;
    ret.data = (char*) data;
    if (ret.data != NULL) {
	ret.len = len;
	ret.capacity = ret.len;
    } else {
	ret.capacity = ret.len = 0;
    }
	 
    return ret;
}


_ystring str_copy_len (const char *data, unsigned long len) {
    char * res = (char*) GC_malloc (len);
    memcpy (res, data, len);
    
    _ystring ret;
    ret.data = res;
    ret.len = len;
    ret.capacity = len;
    
    return ret;
}

_ystring str_copy (const char * data) {
    return str_copy_len (data, strlen (data));
}

_ystring str_fit (_ystring self) {
    return str_copy_len (self.data, self.len);    
}

_ystring str_empty () {
    _ystring str;
    str.data = NULL;
    str.len = 0;
    str.capacity = 0;
    return str;
}

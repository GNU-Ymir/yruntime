#ifndef _Y_DEMANGLE_H_
#define _Y_DEMANGLE_H_

#include <stdint.h>
#include "ystring.h"
#include "yarray.h"

/**
 * Retreive the next number
 */
int _yrt_demangle_number (char * data, int * current);

/**
 * Demangle the name of a symbol
 */
_ystring _yrt_demangle_symbol (char * data, uint64_t len);

/**
 * Demangle the name of a symbol
 */
_yrt_c8_array_ _yrt_demangle_symbol_to_slice (char * data, uint64_t len);

/**
 * Demangle the parameters of a function
 */
_ystring _yrt_demangle_function_parameters (char * data, uint64_t len);

/**
 * Mangle the name of a class 
 */
_yrt_c8_array_ _yrt_mangle_path (_yrt_c8_array_ name);

#endif

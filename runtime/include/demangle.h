#ifndef _Y_DEMANGLE_H_
#define _Y_DEMANGLE_H_

#include "ystring.h"

/**
 * Retreive the next number
 */
int _yrt_demangle_number (char * data, int * current);

/**
 * Demangle the name of a symbol
 */
_ystring _yrt_demangle_symbol (char * data, unsigned long len);

/**
 * Demangle the parameters of a function
 */
_ystring _yrt_demangle_function_parameters (char * data, unsigned long len);

#endif

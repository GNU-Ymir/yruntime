#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_

#include "ystring.h"
#include "yarray.h"

extern int __YRT_DEBUG__;
extern int __YRT_FORCE_DEBUG__;
extern int __YRT_TEST_CODE__;

#ifdef __linux__


/**
 * Find a filename in the PATH env
 * @params:
 *    - filename: the file to find in the PATH
 *    - size: the size of the resolved buffer
 * @returns:
 *    - resolved: the path to the file resolved
 */
char* _yrt_resolve_path (const char* filename, char* resolved, int size);


/**
 * Resolve the address in the obj file
 * @params:
 *    - filename: the name of the file
 *    - addr: the address of the symbol
 * @returns:
 *    - file: the name of the file
 *    - func: the name of the function 
 *    - line: the line of the symbol
 */
int _yrt_resolve_address (const char * filename, void* addr, _ystring * file, int* line);

#endif

/**
 * Retreive the current stack trace 
 * @returns: the stack trace 
 * @warning: the result is allocated with GC, it must not be deallocated
 */
_yrt_array_ _yrt_exc_get_stack_trace ();

/**
 * Transform a list of stacktrace symbols into a list of printable strings
 * @params:
 *   - syms: the stack trace acquired with _yrt_exc_get_stack_trace
 * @returns: a list of string
 */
_yrt_array_ _yrt_exc_resolve_stack_trace (_yrt_array_ syms);


#endif

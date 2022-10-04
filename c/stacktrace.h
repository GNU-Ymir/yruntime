#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_

#include "ystring.h"
#include "yarray.h"

extern int __YRT_DEBUG__;
extern int __YRT_FORCE_DEBUG__;
extern int __YRT_TEST_CODE__;

#ifdef __linux__

#include <bfd.h>

/**
 * The handle of an object file
 */
struct bfd_handle {
    bfd* ptr;
    const char* filename;
    asymbol** syms;
    long nb_syms;
};

/**
 * The result of a bfd address search
 */
struct bfd_context {
    struct bfd_handle handle;
    int set;
    int pc;
    
    const char* file;
    const char* function;
    unsigned int line;
};

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
 * Retreive the symbol table of a bfd handle
 * @params:
 *    - handle: an opened handle
 */
void _yrt_slurp_sym_table (struct bfd_handle * handle);

/**
 * Open a file in bfd format and return the pointer 
 * @params: 
 *    - filename: the path of the obj file
 * @return: the pointer of the readable bfd file
 */
struct bfd_handle _yrt_get_bfd_file (const char * filename);

/**
 * Close all the opened files
 */
void _yrt_close_bfd_file (struct bfd_handle);

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
int _yrt_resolve_address (const char * filename, void* addr, _ystring * file, _ystring * func, int* line, struct bfd_handle handle);

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

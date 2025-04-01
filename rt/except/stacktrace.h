#ifndef STACKTRACE_H_
#define STACKTRACE_H_

#include <rt/utils/string.h>
#include <rt/memory/types.h>

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
int _yrt_resolve_address (const char * filename, void* addr, struct _yrt_reflect_symbol_t ref, _yrt_slice_t * file, int* line);

#endif

/**
 * Retreive the current stack trace
 * @returns: the stack trace
 * @warning: the result is allocated with GC, it must not be deallocated
 */
_yrt_slice_t _yrt_exc_get_stack_trace ();

/**
 * Transform a list of stacktrace symbols into a list of printable strings
 * @params:
 *   - syms: the stack trace acquired with _yrt_exc_get_stack_trace
 * @returns: a list of string
 */
_yrt_slice_t _yrt_exc_resolve_stack_trace (_yrt_slice_t syms);



/*!
 * ====================================================================================================
 * ====================================================================================================
 * =========================          IN core::reflect from midgard          ==========================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Update the elf loader, and search for a symbol from its address
 */
struct _yrt_reflect_symbol_t _yrt_reflect_find_symbol_from_addr_with_elf_name (void * addr, _yrt_slice_t name);

#endif // STACKTRACE_H_

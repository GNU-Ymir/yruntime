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

/**
 * Update the dwarf loader and search for debug information
 */
struct _yrt_reflect_debug_symbol_info_t _yrt_reflect_get_debug_info (_yrt_slice_t name, void * addr, struct _yrt_reflect_symbol_t sym);

/**
 * Clear loaded debug information
 */
void _yrt_reflect_clear_debug_info ();

#endif // STACKTRACE_H_

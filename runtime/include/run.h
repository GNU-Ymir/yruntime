#ifndef _RUN_H_
#define _RUN_H_

#include "ymemory.h"
#include "ystring.h"

#include <bfd.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

/**
 * Retreive the current stack trace 
 * @returns: the stack trace 
 * @warning: the result is allocated with GC, it must not be deallocated
 */
_yrt_array_ _yrt_exc_get_stack_trace ();


/**
 * The real main function of a ymir program
 * Transform the C-like arguments into Ymir usable ones
 * Also install handlers to throw exception when seg_fault is encountered
 * @params: 
 *    - argc: the argc of the C main function
 *    - argv: the argv of the C main function
 *    - y_main: the ymir main function
 * @info: does not init the debugging symbols, nor stack trace retrieval 
 */
int y_run_main (int argc, char ** argv, int (*) ());

/**
 * The real main function of a ymir program
 * Transform the C-like arguments into Ymir usable ones
 * Also install handlers to throw exception when seg_fault is encountered
 * @params: 
 *    - argc: the argc of the C main function
 *    - argv: the argv of the C main function
 *    - y_main: the ymir main function
 * @info: init the debugging symbols, and stack trace retrieval 
 */
int y_run_main_debug (int argc, char ** argv, int (*) ());

/**
 * Display an error on the stderr stream
 * @params: 
 *    - len: the length of the error
 *    - ptr: a string (not necessarily null terminated)
 */
void _y_error (unsigned long len, char * ptr);


#endif

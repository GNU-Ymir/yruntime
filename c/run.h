#ifndef _RUN_H_
#define _RUN_H_

#include "ymemory.h"
#include "ystring.h"

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

/**
 * Exit the program with a given code
 * @params:
 *   - code: the code on exit
 */
void _yrt_exit (int code);

/**
 * Throw the abort exception
 * @params:
 *   - msg: the message of the abort exception
 */
void _yrt_throw_runtime_abort (_yrt_c8_array_ msg);

/**
 * Throw the seg fault exception
 */
void _yrt_throw_seg_fault ();

#endif

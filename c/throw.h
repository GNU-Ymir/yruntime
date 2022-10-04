#ifndef _THROW_H_
#define _THROW_H_

#include <setjmp.h>
#include "type.h"
#include "thread.h"
#include <stdio.h>
    
/* Stack is actually a linked list of catcher cells. */
struct _yrt_exc_stack
{
    jmp_buf j;
    struct _yrt_exc_stack *next;
} ;

/** List of all thread (catching exception at some point, or throwing) */
struct _yrt_thread_stack
{
    _yrt_thread_t id;
    struct _yrt_thread_stack * next;
    struct _yrt_exc_stack * stack;
    
    _ytype_info info;
    void* data;

    char * file;
    char * function;
    unsigned line;
    int code;
} ;

/**
 * Initialize the throwing and catching system
 */
void _yrt_exc_init ();

/**
 * Get the thread stack ptr of the current thread
 * @params: 
 *    - id: the thread
 *    - current: the stack of the exc_stack
 * @return: an iterator on the thread or NULL if not found
 */
struct _yrt_thread_stack* _yrt_get_thread_stack_current (_yrt_thread_t id, struct _yrt_thread_stack* current);

/**
 * Insert a new exc_stack for the thread
 * @param: 
 *    - id: the thread id
 * @return: the new stack 
 * @warning: lock the mutex while inserting
 */
struct _yrt_thread_stack* _yrt_insert_thread (_yrt_thread_t id);

/**
 * Get the iterator on the thread stack for the thread id
 * Insert the stack if not found
 * @params:
 *    - id: the thread we are looking for
 * @returns: an iterator 
 */
struct _yrt_thread_stack* _yrt_get_thread_stack (_yrt_thread_t id);

/**
 * Get the iterator on the thread stack for the thread id
 * @params:
 *    - id: the thread we are looking for
 * @returns: an iterator or NULL if not found
 */
struct _yrt_thread_stack* _yrt_get_thread_stack_no_add (_yrt_thread_t id);

/**
 * Remove the exc_stack of the thread inside the thread stack 
 * @params:
 *    - id: the id of the thread
 *    - current: the current stack
 *    - last: the iterator that leds to current
 */
void _yrt_remove_thread_current (_yrt_thread_t id, struct _yrt_thread_stack* current, struct _yrt_thread_stack** last);

/**
 * Remove the exc_stack of the thread 
 * @params:
 *    - id: the id of the thread
 */
void _yrt_remove_thread (_yrt_thread_t id);

/**
 * Main routine of the exception system
 * @params: 
 *    - j: the reference in the program stack
 *    - returned: are we here due to stack unwind?
 * @return: are we here due to stack unwind?
 */
int _yrt_exc_push (jmp_buf * j, int returned);

/**
 * Pop the program stack jumpers for a given thread
 * @params:
 *    - j: the reference in the program stack to pop
 *    - _list_: the iterator on the exc_stack of the current thread
 */
void _yrt_exc_pop_list (jmp_buf* j, struct _yrt_thread_stack* _list_);

/**
 * Find the current thread and call the pop for the current thread
 */
void _yrt_exc_pop (jmp_buf * j);

/**
 * Check if the type of the current throwed exception is the correct type
 * @params:
 *    - info: the type to catch
 * @return: the ptr to the data or NULL, if the type is incorrect
 */
void* _yrt_exc_check_type (_ytype_info info);

/**
 * Throw an exception 
 * @params:
 *    - file: the file in which the exception is thrown
 *    - function: the function in which the exception is thrown
 *    - line: the line in the source code that triggered the throw
 *    - info: the type of the exception
 *    - data: the data of the exception
 */
void _yrt_exc_throw (char* file, char* function, unsigned int line, _ytype_info info, void* data);

/**
 * Rethrow an exception that was not caught
 */
void _yrt_exc_rethrow ();
    

/**
 * Print the exception in the out stream 
 * @param: 
 *    - stream: the output stream
 *    - list: the list of the current stack trace
 */
void _yrt_exc_print (FILE * stream, struct _yrt_thread_stack * list);

/**
 * Provoke a program panic (print stack trace and exit)
 * @params:
 *    - file: the name of the file panicking
 *    - function: the name of the function panicking
 *    - line: the line of the panic in the source code
 */
void _yrt_exc_panic (const char *file, const char *function, unsigned line);

/**
 * Throw a segmentation fault exception
 * @info: the implementation of this function is in the core files of ymir
 */
void _yrt_throw_seg_fault ();

/**
 * Throw a runtime error
 * @info: the implementation of this function is in the core files of ymir
 */
void _yrt_throw_runtime_abort (_yrt_c8_array_ str);

#endif

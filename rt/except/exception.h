#ifndef EXCEPTION_H_
#define EXCEPTION_H_


#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>
#include <stddef.h>
#include <string.h>

#include <rt/memory/types.h>
#include <rt/concurrency/thread.h>

/**
 * Content of an unwindable exception
 */
typedef struct  _yrt_exception_header_t {
    struct _Unwind_Exception unwindHeader;

    // the Ymir class Exception thrown
    void* object;

    // The id of the thread that created the exception
    pthread_t thread_id;

    // The file containing the exception
    const char* file;

    // The name of the function that threw
    const char* function;

    // The line of the exception
    unsigned long line;

    // Language specific datas
    const char* lsda;

    // The handler found in phase 1
    int handler;

    // The landing pad found in phase 1
    _Unwind_Ptr landingPad;

    // Set in phase 1
    _Unwind_Word cfa;

    // stack other thrown exception in the current thread
    struct _yrt_exception_header_t * next;

} _yrt_exception_header_t;

/**
 * stack of the exceptions (one stack per thread)
 */
typedef struct _yrt_exc_thread_stack_t {
    _yrt_exception_header_t * stack;
    struct _yrt_exc_thread_stack_t * next;

    _yrt_exception_header_t ehstorage;

    pthread_t id;
} _yrt_exc_thread_stack_t;


/**
 * ======================================================================================
 * ======================================================================================
 * ======================================  ALLOC  ======================================
 * ======================================================================================
 * ======================================================================================
 */


/**
 * Initialize exception system
 */
void _yrt_exc_init ();

/**
 * @returns: the stack for the current thread NULL if none
 */
_yrt_exc_thread_stack_t* _yrt_exc_get_thread_stack_current (pthread_t id, _yrt_exc_thread_stack_t * current);

/**
 * Insert a new exception stack into the thread stack
 */
_yrt_exc_thread_stack_t* _yrt_exc_insert_thread (pthread_t id);

/**
 * Remove the exception stack for the thread 'id'
 */
void _yrt_exc_remove_thread (_yrt_exc_thread_stack_t* stack);


/**
 * Get the exception stack of the current thread
 * Insert a new one if not found
 */
_yrt_exc_thread_stack_t* _yrt_exc_get_thread_stack (pthread_t id);

/**
 * Allocate and init an _yrt_exception_header_
 */
_yrt_exception_header_t* _yrt_exc_create_header (void* object, _yrt_exc_thread_stack_t* stack);

/**
 * Free exception created by create ()
 */
void _yrt_exc_free_header (_yrt_exception_header_t* head, _yrt_exc_thread_stack_t* stack);

/**
 * Push the exception header onto the stack
 */
void _yrt_exc_push (_yrt_exception_header_t* e, _yrt_exc_thread_stack_t * stack);

/**
 * Pop the last pushed exception of the stack
 */
_yrt_exception_header_t* _yrt_exc_pop (_yrt_exc_thread_stack_t* stack);


/**
 * ======================================================================================
 * ======================================================================================
 * ======================================  PERSONALITY  =================================
 * ======================================================================================
 * ======================================================================================
 */

/**
 * Terminate the program due to a uncaught exception error
 */
void _yrt_exc_panic_exception (_yrt_exception_header_t* eh);

/**
 * Cast an _Unwind_Exception into a _yrt_exception header
 */
_yrt_exception_header_t* _yrt_to_exception_header (struct _Unwind_Exception* exc);


/**
 * Called by unwinder when exception object needs destruction outside of ymir runtime
 */
void _yrt_exc_exception_cleanup (_Unwind_Reason_Code code, struct _Unwind_Exception* exc);


/**
 * Runtime function called by the throw statement
 */
void _yrt_exc_throw (char *file, char *function, unsigned line, void* data);

/**
 * Runtime function called by the catch statement
 * @returns: the exception that was thrown (in the current thread)
 */
void* _yrt_exc_begin_catch (struct _Unwind_Exception* unwindHeader);



#endif // EXCEPTION_H_

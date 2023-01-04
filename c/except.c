#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>
#include <stddef.h>
#include <string.h>
#include "yarray.h"
#include "type.h"
#include "thread.h"
#include "stacktrace.h"
#include "except.h"


// The local stack of chained exceptions
_yrt_exc_thread_stack_ global_stack;

pthread_mutex_t _yrt_exc_mutex;

void _yrt_exc_init () {
    _yrt_thread_mutex_init (&_yrt_exc_mutex, NULL);
    global_stack.id = pthread_self ();
    global_stack.next = NULL;
    global_stack.stack = NULL;
}


/**
 * ======================================================================================
 * ======================================================================================
 * ======================================  ALLOC  ======================================
 * ======================================================================================
 * ======================================================================================
 */


/**
 * @returns: the stack for the current thread NULL if none
 */
_yrt_exc_thread_stack_* _yrt_exc_get_thread_stack_current (pthread_t id, _yrt_exc_thread_stack_ * current) {
    while (current != NULL) {
	if (pthread_equal (current-> id, id)) return current;
	current = current-> next;
    }

    return NULL;
}


/**
 * Insert a new exception stack into the thread stack
 */
_yrt_exc_thread_stack_* _yrt_exc_insert_thread (pthread_t id) {
    _yrt_exc_thread_stack_ * head = (_yrt_exc_thread_stack_*) __builtin_calloc (sizeof (_yrt_exc_thread_stack_), 1);
    if (head == NULL) {
	_yrt_exc_terminate ("out of memory\n", __LINE__);
    }
    
    head-> id = id;    
    head-> stack = NULL;
    head-> next = NULL;
    
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_exc_thread_stack_* current = &global_stack;
    while (current != NULL) {
	if (current-> next == NULL) {
	    current-> next = head;
	    break;
	}
	current = current-> next;
    }
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);

    return head;
}

/**
 * Remove the exception stack for the thread 'id'
 */
void _yrt_exc_remove_thread (_yrt_exc_thread_stack_* stack) {
    if (stack != &global_stack) {
	_yrt_thread_mutex_lock (&_yrt_exc_mutex);
	// we have to look up the thread in the list as it could have changed since insertion
	_yrt_exc_thread_stack_* current = &global_stack;
	while (current != NULL) {
	    if (current-> next == stack) {
		current-> next = current-> next-> next;
		__builtin_free (stack);
		break;
	    }

	    current = current-> next;	    
	}
	_yrt_thread_mutex_unlock (&_yrt_exc_mutex);
    }
}


/**
 * Get the exception stack of the current thread
 * Insert a new one if not found
 */
_yrt_exc_thread_stack_* _yrt_exc_get_thread_stack (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_exc_thread_stack_* got = _yrt_exc_get_thread_stack_current (id, &global_stack);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);

    if (got == NULL) {
	return _yrt_exc_insert_thread (id);
    }

    return got;
}

/**
 * Allocate and init an _yrt_exception_header_
 */
_yrt_exception_header_* _yrt_exc_create_header (void* object, _yrt_exc_thread_stack_* stack) {    
    _yrt_exception_header_* eh = &(stack-> ehstorage);
    if (eh-> object != NULL) { // pre allocate already in use
	eh = (_yrt_exception_header_*) __builtin_calloc (sizeof (_yrt_exception_header_), 1);
	if (!eh) {
	    _yrt_exc_terminate ("out of memory\n", __LINE__);
	}
    }

    eh-> object = object;
    eh-> thread_id = stack-> id;

    return eh;
}

/**
 * Free exception created by create ()
 */
void _yrt_exc_free_header (_yrt_exception_header_* head, _yrt_exc_thread_stack_* stack) {
    memset (head, 0, sizeof (_yrt_exception_header_));
    if (head != &(stack-> ehstorage)) {
	__builtin_free (head);
    }
    
    if (stack-> stack == NULL) {
	_yrt_exc_remove_thread (stack);
    }
}

/**
 * Push the exception header onto the stack
 */
void _yrt_exc_push (_yrt_exception_header_* e, _yrt_exc_thread_stack_ * stack) {
    e-> next = stack-> stack;
    stack-> stack = e;
}

/**
 * Pop the last pushed exception of the stack
 */
_yrt_exception_header_* _yrt_exc_pop (_yrt_exc_thread_stack_* stack) {
    _yrt_exception_header_* eh = stack-> stack;
    stack-> stack = eh-> next;
    return eh;
}


/**
 * ======================================================================================
 * ======================================================================================
 * ======================================  PERSONALITY  =================================
 * ======================================================================================
 * ======================================================================================
 */


/**
 * Terminate the program due to a irrecoverable error
 */
void _yrt_exc_terminate (const char * msg, unsigned int line) {
    static char terminating = 0;
    if (terminating) {
	fprintf (stderr, "terminating called recursively\n");
	abort ();
    }

    terminating = 1;
    fprintf (stderr, "gcc.deh(%u): %s\n", line, msg);
    _yrt_array_ trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
	fprintf (stderr, "%s\n", (char*) trace.data);
    }
    
    abort ();
}

void _yrt_exc_panic_exception (_yrt_exception_header_* eh)
{
    fprintf (stderr, "Panic in file \"%s\", at line %lu", eh-> file, eh-> line);
    fprintf (stderr, ", in function \"%s\" !!! \n", eh-> function);
    _yrt_array_ trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
	fprintf (stderr, "%s\n", (char*) trace.data);
    }

    exit (-1);
}

void _yrt_exc_panic (const char* file, const char * function, unsigned int line) {
    fprintf (stderr, "Panic in file \"%s\", at line %u", file, line);
    fprintf (stderr, ", in function \"%s\" !!! \n", function);
    _yrt_array_ trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
	fprintf (stderr, "%s\n", (char*) trace.data);
    }

    exit (-1);
}


_yrt_exception_header_* _yrt_to_exception_header (struct _Unwind_Exception* exc) {
    // the unwindHeader is the first field of the _yrt_exception_header_
    // So it has the same address
    return (_yrt_exception_header_*) ((void*) exc);
}


/**
 * Called by unwinder when exception object needs destruction outside of ymir runtime
 */
void _yrt_exc_exception_cleanup (_Unwind_Reason_Code code, struct _Unwind_Exception* exc) {
    _yrt_exception_header_* eh = _yrt_to_exception_header (exc);
    if (code != _URC_FOREIGN_EXCEPTION_CAUGHT && code != _URC_NO_REASON) {
	_yrt_exc_panic_exception (eh);
    }

    _yrt_exc_thread_stack_* stack = _yrt_exc_get_thread_stack (eh-> thread_id);
    _yrt_exc_free_header (eh, stack);
}


void _yrt_exc_throw (char *file, char *function, unsigned line, void* data) {
    _yrt_exc_thread_stack_* stack = _yrt_exc_get_thread_stack (pthread_self ());
    _yrt_exception_header_* eh = _yrt_exc_create_header (data, stack);
    
    eh-> file = file;
    eh-> function = function;
    eh-> line = line;

    
    // Add to thrown exception stack
    _yrt_exc_push (eh, stack);

    eh-> unwindHeader.exception_cleanup = &_yrt_exc_exception_cleanup;
    _Unwind_Reason_Code r = _Unwind_RaiseException (&eh-> unwindHeader);

    if (r == _URC_END_OF_STACK) {
	_yrt_exc_panic_exception (eh);
    }

    _yrt_exc_terminate ("unwind error ", __LINE__);
}

void* _yrt_exc_begin_catch (struct _Unwind_Exception* unwindHeader) {
    _yrt_exc_thread_stack_* stack = _yrt_exc_get_thread_stack (pthread_self ());
    _yrt_exception_header_* header = _yrt_to_exception_header (unwindHeader);
    void* object = header-> object;    
    
    // Something went wront when stacking headers
    if (header != _yrt_exc_pop (stack)) {
	_yrt_exc_terminate ("Catch error", __LINE__);
    }

    // The exception handling is complete
    _Unwind_DeleteException (&header-> unwindHeader);

    return object;
}

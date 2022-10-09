#include <setjmp.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "throw.h"
#include "print.h"
#include "stacktrace.h"


/* This is the global stack of catchers. */
struct _yrt_thread_stack *_yrt_exc_global = NULL;
pthread_mutex_t _yrt_exc_mutex;


void _yrt_exc_init () {
    _yrt_thread_mutex_init (&_yrt_exc_mutex, NULL);
}

void _yrt_exc_panic (const char *file, const char *function, unsigned line)
{
    fprintf (stderr, "Panic in file \"%s\", at line %u", file, line);
    fprintf (stderr, ", in function \"%s\" !!! \n", function);
    _yrt_array_ trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
	fprintf (stderr, "%s\n", (char*) trace.data);
    }
    exit (-1);
}

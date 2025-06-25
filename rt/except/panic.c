#include "panic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>
#include <stddef.h>
#include <string.h>

#include <rt/memory/types.h>
#include <rt/except/stacktrace.h>

/**
 * Terminate the program due to a irrecoverable error
 */
void _yrt_exc_terminate (const char * msg, const char * file, const char * func, unsigned int line) {
    static char terminating = 0;
    if (terminating) {
        fprintf (stderr, "terminating called recursively\n");
        abort ();
    }

    terminating = 1;
    fprintf (stderr, "terminate (%s/%s:%u): %s\n", file, func, line, msg);
    _yrt_slice_t trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
        fprintf (stderr, "%s\n", (char*) trace.data);
    }

    abort ();
}


void _yrt_exc_panic_no_trace () {
    fprintf (stderr, "Panic during stacktrace !");
    abort ();
}

void _yrt_exc_panic (const char* file, const char * function, unsigned int line) {
    fprintf (stderr, "Panic in file \"%s\", at line %u", file, line);
    fprintf (stderr, ", in function \"%s\" !!! \n", function);
    _yrt_slice_t trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
        fprintf (stderr, "%s\n", (char*) trace.data);
    }

    abort ();
}

void _yrt_exc_panic_seg_fault () {
    fprintf (stderr, "Segfault - ");
    _yrt_slice_t trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0) {
        fprintf (stderr, "%s\n", (char*) trace.data);
    }

    fprintf (stderr, "\n");
    abort ();
}

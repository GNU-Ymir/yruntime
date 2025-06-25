#ifndef PANIC_H_
#define PANIC_H_

/**
 * Terminate the program due to a irrecoverable error with an error message
 */
void _yrt_exc_terminate (const char * msg, const char * file, const char * func, unsigned int line);

/**
 * Terminate the program with a panic signal
 */
void _yrt_exc_panic (const char* file, const char * function, unsigned int line);

/**
 * Terminate the program with a panic signal without computing the stack trace
 */
void _yrt_exc_panic_no_trace ();

/**
 * Terminate the program with a panic signal without computing the stack trace
 */
void _yrt_exc_panic_seg_fault ();

#endif // PANIC_H_

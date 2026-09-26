#ifndef CAPTURE_H_
#define CAPTURE_H_

#include <rt/memory/types.h>
#include <stdio.h>

/**
 * The stream the calling thread prints to: its capture stream while one is active(see
 * _yrt_capture_begin), stdout otherwise
 * */
FILE * _yrt_stdout ();

/**
 * The stream the calling thread prints errors to: its capture stream while one is active, stderr
 * otherwise -- stdout and stderr are captured into the same stream, so their order is kept
 * */
FILE * _yrt_stderr ();

/**
 * Start capturing what the calling thread, and the threads it spawns from now on, print to stdout
 * and stderr. Captures nest, the innermost one receives the output.
 * */
void _yrt_capture_begin ();

/**
 * Stop the innermost capture of the calling thread
 * @returns: what was printed since the matching _yrt_capture_begin, the output of a spawned thread
 *           still running afterwards goes to stdout
 * */
_yrt_slice_t _yrt_capture_end ();

/**
 * @returns: the capture of the calling thread, retained for a thread about to be spawned
 * */
void * _yrt_i_capture_retain ();

/**
 * Release a capture retained by _yrt_i_capture_retain, `capture` may be null
 * */
void _yrt_i_capture_release (void * capture);

/**
 * Make `capture`(see _yrt_i_capture_retain) the capture of the calling thread
 * */
void _yrt_i_capture_adopt (void * capture);

/**
 * Release the capture adopted by the calling thread(see _yrt_i_capture_adopt)
 * */
void _yrt_i_capture_drop ();

#endif // CAPTURE_H_

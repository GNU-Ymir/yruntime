#ifndef _STACK_H_
#define _STACK_H_

#include <stdint.h>

/**
 * @returns: the number of bytes left on the stack of the calling thread below the frame of the caller,
 * or UINT64_MAX when the bounds of the stack are unknown
 * @info: the bounds are looked up on the first call of each thread only
 */
uint64_t _yrt_stack_left ();

#endif

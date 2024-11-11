#ifndef _Y_STRING_H_
#define _Y_STRING_H_

#include <stdint.h>
#include <rt/memory/types.h>

/**
 * Transform a int value into a string
 */
_yrt_slice_ str_from_int (int32_t value);

/**
 * Transform a int value into a string
 */
_yrt_slice_ str_from_char (char value);

/**
 * Transform a ptr value into a string
 */
_yrt_slice_ str_from_ptr (void* value);

/**
 * Create a _yrt_slice_ pointer to data
 */
_yrt_slice_ str_create (const char * data);

/**
 * Create a _yrt_slice_ pointer to data
 */
_yrt_slice_ str_create_len (const char * data, unsigned long len);

/**
 * Create a copy of the data
 */
_yrt_slice_ str_copy_len (const char *data, unsigned long len);

/**
 * Create a copy of the data
 * @warning: the data is assumed to be null terminated
 */
_yrt_slice_ str_copy (const char * data);

/**
 * Create an empty string
 */
_yrt_slice_ str_empty ();

#endif

#ifndef CONV_H_
#define CONV_H_

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <rt/memory/types.h>

/**
 * Convert a uint64_t into a double
 * */
double _yrt_u64_to_double (uint64_t x);

/**
 * Convert a ptr into a u64
 * */
uint64_t _yrt_ptr_to_u64 (void * x);

/**
 * Convert a int64_t into a double
 * */
double _yrt_i64_to_double (int64_t x);

/**
 * Convert a uint32_t into a float
 * */
float _yrt_u32_to_float (uint32_t x);

/**
 * Convert a int32_t into a float
 * */
float _yrt_i32_to_float (int x);

/**
 * Convert a double into a uint64_t
 *  */
uint64_t _yrt_double_to_u64 (double x);

/**
 * Convert a double into a int64_t
 *  */
int64_t _yrt_double_to_i64 (double x);

/**
 * Convert a float into a u32
 *  */
uint32_t _yrt_float_to_u32 (float x);

/**
 * Convert a float into a i32
 *  */
int32_t _yrt_float_to_i32 (float x);

/**
 * Convert a string into a float
 *  */
float _yrt_s8_to_float (_yrt_slice_t arr, char * succ);

/**
 * Convert a string into a double
 * */
double _yrt_s8_to_double (_yrt_slice_t arr, char * succ);

_yrt_slice_t _yrt_double_to_s8 (double x, int prec);
_yrt_slice_t _yrt_double_to_s8_exp (double x, int prec);


/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          UTF32 to UTF8          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

_yrt_slice_t _yrt_to_utf8_slice (_yrt_slice_t array);
char* _yrt_to_utf8 (uint32_t code, char chars [5], int * nb);
size_t utf8_codepoint_size (char c);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          UTF8 to UTF32          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

_yrt_slice_t _yrt_to_utf32_slice (_yrt_slice_t array);
uint32_t _yrt_to_utf32 (char* text, size_t * byte_count);

#endif

#ifndef CONV_H_
#define CONV_H_

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <rt/memory/types.h>

/*!
 * ====================================================================================================
 * ====================================================================================================
 * ======================================          HASH          ======================================
 * ====================================================================================================
 * ====================================================================================================
 */

/**
 * Convert a ptr into a u64
 * */
uint64_t _yrt_ptr_to_u64 (void * x);

/**
 * Convert a delegate ptr into a u64
 * */
uint64_t _yrt_dg_to_u64 (void * closure, void * func);


/*!
 * ====================================================================================================
 * ====================================================================================================
 * ============================          FLOAT TO STRING TO FLOAT          ============================
 * ====================================================================================================
 * ====================================================================================================
 */

_yrt_slice_t  _yrt_f32_to_string (float f, uint32_t prec);
_yrt_slice_t  _yrt_f64_to_string (double f, uint32_t prec);
_yrt_slice_t  _yrt_f80_to_string (long double f, uint32_t prec);
_yrt_slice_t  _yrt_fsize_to_string (long double f, uint32_t prec);

_yrt_slice_t  _yrt_f32_to_string_exp (float f, uint32_t prec);
_yrt_slice_t  _yrt_f64_to_string_exp (double f, uint32_t prec);
_yrt_slice_t  _yrt_f80_to_string_exp (long double f, uint32_t prec);
_yrt_slice_t  _yrt_fsize_to_string_exp (long double f, uint32_t prec);

float _yrt_string_to_f32 (_yrt_slice_t str, uint8_t * succ);
double _yrt_string_to_f64 (_yrt_slice_t str, uint8_t * succ);
long double _yrt_string_to_f80 (_yrt_slice_t str, uint8_t * succ);
long double _yrt_string_to_fsize (_yrt_slice_t str, uint8_t * succ);

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          UTF32 to UTF8          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

char* _yrt_to_utf8 (uint32_t code, char chars [5], int * nb);
size_t utf8_codepoint_size (char c);

#endif

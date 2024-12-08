#include <rt/memory/conv.h>


#include <rt/utils/gc.h>
#include <rt/memory/alloc.h>

#include <stdio.h>

double _yrt_u64_to_double (uint64_t x) {
    return (double) x;
}

uint64_t _yrt_ptr_to_u64 (void * x) {
	return (uint64_t) x;
}

uint64_t _yrt_dg_to_u64 (void * closure, void * ptr) {
    uint64_t p = 31;
    uint64_t m = 1000000009;
    uint64_t res = 0;
    uint64_t p_pow = 1;

    res = (res + ((uint64_t) (closure) + 1) * p_pow) % m;
    p_pow = (p_pow * p) % m;
    res = (res + ((uint64_t) (ptr) + 1) * p_pow) % m;

    return res;
}

double _yrt_i64_to_double (int64_t x) {
    return (double) x;
}

float _yrt_u32_to_float (uint32_t x) {
    return (float) x;
}

float _yrt_i32_to_float (int32_t x) {
    return (float) x;
}

uint64_t _yrt_double_to_u64 (double x) {
    return (uint64_t) x;
}

int64_t _yrt_double_to_i64 (double x) {
    return (uint64_t) x;
}

uint32_t _yrt_float_to_u32 (float x) {
    return (unsigned int) x;
}

int32_t _yrt_float_to_i32 (float x) {
    return (int) x;
}

float _yrt_s8_to_float (_yrt_slice_t arr, char * succ) {
    char* endf;
    float res = strtof (arr.data, &endf);
    *succ = (errno != ERANGE) && endf == (arr.data + arr.len);
    return res;
}

double _yrt_s8_to_double (_yrt_slice_t arr, char * succ) {
    char* endf;
    double res = strtod (arr.data, &endf);
    *succ = (errno != ERANGE) && endf == (arr.data + arr.len);
    return res;
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          UTF32 TO UTF8          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

_yrt_slice_t _yrt_to_utf8_slice (_yrt_slice_t array) {
    uint64_t len = 0;
    for (uint64_t i = 0 ; i < array.len ; i++) {
		int nb = 0;
		char buf [5];
		_yrt_to_utf8 (((uint32_t*) array.data) [i], buf, &nb);
		len += nb;
    }

	_yrt_slice_t result;
	_yrt_alloc_slice_no_set (&result, len, 1);
	uint32_t offset = 0;
    for (unsigned long i = 0 ; i < array.len; i++) {
		int nb = 0;
		_yrt_to_utf8 (((uint32_t *) array.data) [i], result.data + offset, &nb);
		offset += nb;
    }

    return result;
}

char* _yrt_to_utf8 (unsigned int code, char chars[5], int * nb) {
    if (code <= 0x7F) {
		chars[0] = (code & 0x7F); chars[1] = '\0';
		*nb = 1;
    } else if (code <= 0x7FF) {
		// one continuation byte
		chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[0] = 0xC0 | (code & 0x1F);
		chars[2] = '\0';
		*nb = 2;
    } else if (code <= 0xFFFF) {
		// two continuation bytes
		chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[0] = 0xE0 | (code & 0xF); chars[3] = '\0';
		*nb = 3;
    } else if (code <= 0x10FFFF) {
		// three continuation bytes
		chars[3] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[2] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[1] = 0x80 | (code & 0x3F); code = (code >> 6);
		chars[0] = 0xF0 | (code & 0x7); chars[4] = '\0';
		*nb = 4;
    } else {
		// unicode replacement character
		chars[2] = 0xEF; chars[1] = 0xBF; chars[0] = 0xBD;
		chars[3] = '\0';
		*nb = 3;
    }
    return chars;
}

size_t utf8_codepoint_size (char c) {
    if((c & 0b10000000) == 0) {
		return 1;
    }

    if((c & 0b11100000) == 0b11000000) {
		return 2;
    }

    if((c & 0b11110000) == 0b11100000) {
		return 3;
    }

    return 4;
}

/*!
 * ====================================================================================================
 * ====================================================================================================
 * =================================          UTF8 TO UTF32          ==================================
 * ====================================================================================================
 * ====================================================================================================
 */

uint32_t _yrt_to_utf32 (char* text, size_t * byte_count) {
    *byte_count = utf8_codepoint_size(text[0]);

    uint a = 0, b = 0, c = 0, d = 0;
    uint a_mask, b_mask, c_mask, d_mask;
    a_mask = b_mask = c_mask = d_mask = 0b00111111;

    switch(*byte_count) {
    case 4 : {
		a = text [0]; b = text [1]; c = text [2]; d = text [3];
		a_mask = 0b00000111;
    } break;
    case 3 : {
		b = text [0]; c = text [1]; d = text [2];
		b_mask = 0b00001111;
    } break;
    case 2 : {
		c = text [0]; d = text [1];
		c_mask = 0b00011111;
    } break;

    case 1 : {
		d = text [0];
		d_mask = 0b01111111;
    } break;
    }

    uint b0 = a & a_mask;
    uint b1 = b & b_mask;
    uint b2 = c & c_mask;
    uint b3 = d & d_mask;
    return ((b0 << 18) | (b1 << 12) | (b2 << 6) | b3);
}


_yrt_slice_t _yrt_to_utf32_slice (_yrt_slice_t array) {
    uint64_t len = 0;
    for (uint64_t i = 0 ; i < array.len ;) {
		size_t nb = 0;
		_yrt_to_utf32 (array.data + i, &nb);
		i += nb;
		len += 1;
    }

	_yrt_slice_t result;
	_yrt_alloc_slice_no_set (&result, len, 1);
    int j = 0;
    for (uint64_t i = 0 ; i < array.len ;) {
		size_t nb = 0;
		((uint32_t*) result.data) [j] = _yrt_to_utf32 (array.data + i, &nb);
		i += nb;
		j += 1;
    }

    return result;
}

_yrt_slice_t _yrt_double_to_s8 (double x, int prec) {
	int nb = snprintf (NULL, 0, "%.*lf", prec, x);

	_yrt_slice_t result;
	_yrt_alloc_slice_no_set (&result, nb + 1, 1);	 // + 1 for the null char
    snprintf (result.data, nb+1, "%.*lf", prec, x);

    return result;
}

_yrt_slice_t _yrt_double_to_s8_exp (double x, int prec) {
    int nb = snprintf (NULL, 0, "%.*e", prec, x);

	_yrt_slice_t result;
	_yrt_alloc_slice_no_set (&result, nb + 1, 1);	 // + 1 for the null char
    snprintf (result.data, nb+1, "%.*e", prec, x);

    return result;
}

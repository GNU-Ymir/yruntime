#include <stdio.h>
#include "../include/print.h"
#include "../include/array.h"
#include <gc/gc.h>

typedef unsigned int uint;

_yrt_c8_array_ _yrt_to_utf8_array (_yrt_c32_array_ array) {
    unsigned long len = 0;
    for (unsigned long i = 0 ; i < array.len ; i++) {
	int nb = 0;
	char buf[5];
	_yrt_to_utf8 (array.data [i], buf, &nb);
	len += nb;
    }

    char* result = GC_malloc (len * sizeof (char) + 1);
    int offset = 0;
    for (unsigned long i = 0 ; i < array.len; i++) {
	int nb = 0;
	_yrt_to_utf8 (array.data [i], result + offset, &nb);
	offset += nb;
    }

    _yrt_c8_array_ arr;
    arr.data = result;
    arr.len = len;
    
    return arr;
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

unsigned int _yrt_to_utf32 (char* text) {
    size_t byte_count = utf8_codepoint_size(text[0]);
			
    uint a = 0, b = 0, c = 0, d = 0;
    uint a_mask, b_mask, c_mask, d_mask;
    a_mask = b_mask = c_mask = d_mask = 0b00111111;
			
    switch(byte_count) {
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

void _yrt_putwchar (unsigned int code) {
    char c[5];
    int nb = 0;
    printf ("%s", _yrt_to_utf8 (code, c, &nb));    
}

void _yrt_printf32 (float x) {
    printf ("%f", x);
}

void _yrt_printf64 (double x) {
    printf ("%lf", x);
}

unsigned int _yrt_getwchar () {
    char c[5];
    int ig = scanf ("%c", &c[0]);
    size_t size = utf8_codepoint_size (c[0]);
    for (int i = 1 ; i < (int) size ; i++) {
	int ig_ = scanf ("%c", &c[i]);
    }
    
    return _yrt_to_utf32 (c);
}

unsigned int _yrt_getwchar_in_file (FILE * file) {
    char c[5];
    c[0] = fgetc (file);
    size_t size = utf8_codepoint_size (c[0]);
    for (int i = 1 ; i < (int) size; i ++)
	c[i + 1] = fgetc (file);
    return _yrt_to_utf32 (c);
}

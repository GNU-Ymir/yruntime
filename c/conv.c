#include "yarray.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
 

double _yrt_ulong_to_double (unsigned long x) {
    return (double) x;
}

double _yrt_long_to_double (long x) {
    return (double) x;
}

float _yrt_uint_to_float (unsigned int x) {
    return (float) x;
}

float _yrt_int_to_float (int x) {
    return (float) x;
}

unsigned long _yrt_double_to_ulong (double x) {
    return (unsigned long) x;
}

long _yrt_double_to_long (double x) {
    return (long) x;
}

unsigned int _yrt_float_to_uint (float x) {
    return (unsigned int) x;
}

int _yrt_float_to_int (float x) {
    return (int) x;
}

float _yrt_s8_to_float (_yrt_c8_array_ arr, char * succ) {
    char* endf;
    float res = strtof (arr.data, &endf);
    *succ = (errno != ERANGE) && endf == (arr.data + arr.len);
    return res;
}

double _yrt_s8_to_double (_yrt_c8_array_ arr, char * succ) {
    char* endf;
    double res = strtod (arr.data, &endf);
    *succ = (errno != ERANGE) && endf == (arr.data + arr.len);
    return res;
}

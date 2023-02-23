#ifndef _Y_ARRAY_H_
#define _Y_ARRAY_H_

#include <stdint.h>

typedef struct {
    uint64_t len;
    void * data;
} _yrt_array_;

typedef struct {
    uint64_t len;
    char * data;
} _yrt_c8_array_;

typedef struct {
    uint64_t len;
    unsigned int * data;
} _yrt_c32_array_;

/** Transform a [c32] into a [c8] */
_yrt_c8_array_ _yrt_to_utf8_array (_yrt_c32_array_ a);

#endif

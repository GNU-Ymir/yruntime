#ifndef _Y_ARRAY_H_
#define _Y_ARRAY_H_

typedef struct {
    unsigned long long len;
    void * data;
} _yrt_array_;

typedef struct {
    unsigned long long len;
    char * data;
} _yrt_c8_array_;

typedef struct {
    unsigned long long len;
    unsigned int * data;
} _yrt_c32_array_;

/** Transform a [c32] into a [c8] */
_yrt_c8_array_ _yrt_to_utf8_array (_yrt_c32_array_ a);

#endif

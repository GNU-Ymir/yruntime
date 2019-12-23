#ifndef _ARRAY_H_
#define _ARRAY_H_


struct Array {
    unsigned long len;
    void * data;
};

extern "C" Array _yrt_dup_slice (Array, unsigned long);

#endif

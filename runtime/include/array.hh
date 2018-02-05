#ifndef _ARRAY_H_
#define _ARRAY_H_


struct Array {
    unsigned long len;
    void * data;
};

extern "C" void* _y_newArray (unsigned long, unsigned long);

#endif

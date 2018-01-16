#ifndef _ARRAY_H_
#define _ARRAY_H_

union value {
    unsigned char ub;
    char b;
    unsigned short us;
    short s;
    unsigned int ui;
    int i;
    unsigned long ul;
    long l;
    float f;
    double d;
    void* allocated = 0;
};

struct TypeInfo {    
    value val;
    unsigned long size;
};

extern "C" TypeInfo _y_Init_int ();
extern "C" TypeInfo _y_Init_float ();

extern "C" void* _y_newArray (TypeInfo (*) (), unsigned long);

#endif

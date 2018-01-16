#include <array.hh>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <gc/gc.h>
#include <algorithm>

extern "C" TypeInfo _y_Init_char () {
    value val;
    val.b = 0;
    TypeInfo ret = {val, sizeof(char)};
    return ret;
}

extern "C" TypeInfo _y_Init_ubyte () {
    value val;
    val.b = 0;
    TypeInfo ret = {val, sizeof(unsigned char)};
    return ret;
}

extern "C" TypeInfo _y_Init_byte () {
    value val;
    val.b = 0;
    TypeInfo ret = {val, sizeof(char)};
    return ret;
}

extern "C" TypeInfo _y_Init_ushort () {
    value val;
    val.us = 0;
    TypeInfo ret = {val, sizeof(unsigned short)};
    return ret;
}

extern "C" TypeInfo _y_Init_short () {
    value val;
    val.s = 0;
    TypeInfo ret = {val, sizeof(short)};
    return ret;
}

extern "C" TypeInfo _y_Init_uint () {
    value val;
    val.ui = 0;
    TypeInfo ret = {val, sizeof(int)};
    return ret;
}

extern "C" TypeInfo _y_Init_int () {
    value val;
    val.i = 0;
    TypeInfo ret = {val, sizeof(int)};
    return ret;
}

extern "C" TypeInfo _y_Init_ulong () {
    value val;
    val.ul = 0;
    TypeInfo ret = {val, sizeof(long)};
    return ret;
}

extern "C" TypeInfo _y_Init_long () {
    value val;
    val.l = 0;
    TypeInfo ret = {val, sizeof(unsigned long)};
    return ret;
}

extern "C" TypeInfo _y_Init_float () {
    value val;
    val.d = NAN;
    TypeInfo ret = {val, sizeof(float)};
    return ret;
}

extern "C" TypeInfo _y_Init_double () {
    value val;
    val.d = NAN;
    TypeInfo ret = {val, sizeof(double)};
    return ret;
}

extern "C" TypeInfo _y_Init_string () {
    TypeInfo ret = {0, sizeof (unsigned long) + sizeof (char*)};
    return ret;
}

extern "C" void* _y_newArray (TypeInfo (*func) (), ulong len) { 
    auto info = func ();
    auto x = GC_malloc (len * info.size); 
    if (info.size <= 8) {
        switch (info.size) {
        case 1: std::fill ((char*)x, (char*) (x) + len, info.val.b); break;
        case 2: std::fill ((short*)x, (short*) (x) + len, info.val.s); break;
        case 4: std::fill ((int*)x, (int*) (x) + (len), info.val.i); break;
        case 8: std::fill ((long*)x, (long*) (x) + (len), info.val.l); break;
        }
    } else {
	if (info.val.allocated != NULL) {
	    for (unsigned long i = 0 ; i < len ; i++) {
		memcpy ((char*) (x) + (i * info.size), info.val.allocated, info.size); 
	    }
	    free (info.val.allocated);
	} else {
	    std::fill_n ((char*) x, len * info.size, 0);
	}
    }
    return x;
}

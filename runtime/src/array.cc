#include <array.hh>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <gc/gc.h>
#include <algorithm>

extern "C" void* _y_newArray (ulong size, ulong len) { 
    auto x = GC_malloc (len * size);
    std::fill_n ((char*) x, len * size, 0);	    
    return x;
}

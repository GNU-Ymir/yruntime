#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
#include <sys/wait.h>
#include <gc/gc.h>

extern "C" void* _yrt_read_pipe (int stream, ulong size) {
    void * z;
    void ** x = &z; 
    read (stream, x, size);
    return *x;
}

extern "C" void _yrt_write_pipe (int stream, void * data, ulong size) {    
    write (stream, &data, size);
}

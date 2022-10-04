#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <string.h> 
//#include <sys/wait.h>
#include <pthread.h>

#define GC_PTHREADS
#include <gc/gc.h>
#include <semaphore.h>

void _yrt_disable_GC () {
    GC_disable ();
}

void _yrt_enable_GC () {
    GC_enable ();
}

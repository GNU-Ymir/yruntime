#ifndef GC_H_
#define GC_H_

#include <sys/types.h>
#include <pthread.h>

#define GC_PTHREADS
#include <gc/gc.h>
#include <semaphore.h>

#include <gc/gc_disclaim.h>

void _yrt_disable_GC ();
void _yrt_enable_GC ();


#endif // GC_H_

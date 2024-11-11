#include <rt/utils/gc.h>

void _yrt_disable_GC () {
    GC_disable ();
}

void _yrt_enable_GC () {
    GC_enable ();
}

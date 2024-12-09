#include <rt/utils/gc.h>

void _yrt_disable_GC () {
    GC_disable ();
}

void _yrt_enable_GC () {
    GC_enable ();
}

uint8_t _yrt_is_GC_enabled () {
    return !GC_is_disabled ();
}

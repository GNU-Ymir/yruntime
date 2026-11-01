#include <rt/utils/gc.h>

void _yrt_disable_GC () {
    _yrt_nh_disable ();
}

void _yrt_enable_GC () {
    _yrt_nh_enable ();
}

uint8_t _yrt_is_GC_enabled () {
    return _yrt_nh_is_enabled ();
}

void _yrt_gc_collect () {
    _yrt_nh_collect ();
}

void _yrt_gc_wait_finalizers () {
    _yrt_nh_wait_finalizers ();
}

#include <gc/gc.h>
#include "../c/yarray.h"

extern int _yrt_get_test_code ();

extern int _yrt_test_main ();

extern _yrt_array_ _yrt_create_args_array (int len, char ** argv);

void** __MAP__ = 0;

void** _yrt_get_test_map () {
    return __MAP__;
}

void _yrt_set_test_map (void * map) {
    __MAP__ = GC_malloc (sizeof (void*));
    *__MAP__ = map;
}

int main (int argc, char** argv) {
    return _yrt_test_main (_yrt_create_args_array (argc, argv));   
}

#include <gc/gc.h>
#include "../c/yarray.h"

extern int _yrt_get_test_code ();

extern int _yrt_test_main ();

extern _yrt_array_ _yrt_create_args_array (int len, char ** argv);

void** __YRT_TEST_RT_MAP__ = 0;

int main (int argc, char** argv) {
    return _yrt_test_main (_yrt_create_args_array (argc, argv));   
}

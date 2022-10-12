#include "run.h"
#include "ymemory.h"
#include "except.h"
#include "demangle.h"
#include "reflect.h"
#include "stacktrace.h"
#include <gc/gc_disclaim.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>


int __YRT_DEBUG__ = 0;
int __YRT_FORCE_DEBUG__ = 0;
int __YRT_TEST_CODE__ = 0;
_yrt_array_ __MAIN_ARGS__;


void _yrt_exit (int i) {
    exit (i);
}

void bt_sighandler(int sig
#ifdef __linux__
		   , struct sigcontext ctx
#endif
    )
{    
    static int second = 0;
    if (second == 1) {
	char * c = NULL;
	*c = 'i';
    }
    second = 1;
    _yrt_throw_seg_fault ();    
}

void installHandler () {
#ifdef __linux__
    /* struct sigaction sa; */

    /* sa.sa_handler = (void (*)(int))bt_sighandler; */
    /* sigemptyset(&sa.sa_mask); */
    /* sa.sa_flags = SA_RESTART; */

    /* sigaction(SIGSEGV, &sa, NULL); // On seg fault we throw an exception */
#elif _WIN32
    signal (SIGSEGV, &bt_sighandler);
#endif
}

void _yrt_force_debug (int act) {
    if (act == 0) __YRT_DEBUG__ = __YRT_FORCE_DEBUG__;
    else {
	__YRT_FORCE_DEBUG__ = __YRT_DEBUG__;
	__YRT_DEBUG__ = 1;
    }
#ifdef __linux__
    bfd_init ();
#endif
}

_yrt_array_ _yrt_create_args_array (int len, char ** argv) {
    _yrt_array_ arr;
    arr.data = GC_malloc (sizeof (_yrt_array_) * len);
    arr.len = (unsigned long) len;

    for (int i = 0 ; i < len ; i++) {
	_yrt_array_ inner;
	inner.data = argv[i];
	inner.len = strlen (argv [i]);
	((_yrt_array_*) arr.data)[i] = inner;
    }

    __MAIN_ARGS__ = arr;    
    return arr;
}

_yrt_array_ _yrt_get_main_args () {
    return __MAIN_ARGS__;
}

int _yrt_get_test_code () {
    return __YRT_TEST_CODE__;
}

void _yrt_set_test_code (int i) {
    __YRT_TEST_CODE__ = i;
}

int _yrt_run_main_debug (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 1;
    
    GC_INIT ();
#ifdef __linux__
    bfd_init ();
#endif

    installHandler ();
    _yrt_exc_init ();
            
    int ret = y_main (_yrt_create_args_array (argc, argv));
    return ret;
}

int _yrt_run_main (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 0;

    GC_INIT ();
    
    installHandler ();
    _yrt_exc_init ();
    
    
    int ret = y_main (_yrt_create_args_array (argc, argv));    
    return ret;
}

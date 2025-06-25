#include <rt/run.h>

#include <rt/utils/_.h>
#include <rt/except/_.h>
#include <rt/except/panic.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>


int __YRT_DEBUG__ = 0;
int __YRT_FORCE_DEBUG__ = 0;
int __YRT_TEST_CODE__ = 0;
_yrt_slice_t __MAIN_ARGS__;


void _yrt_exit (int i) {
    exit (i);
}

void bt_sighandler(int sig
#ifdef __linux__
                   , struct sigcontext ctx
#endif
                   )
{
    static int first = 0;
    if (first == 0) {
        first = 1;
        _yrt_exc_panic_seg_fault ();
    } else {
        _yrt_exc_panic_no_trace ();
    }
}

void installHandler () {
#ifdef __linux__
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL); // On seg fault we throw an exception
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
}

_yrt_slice_t _yrt_create_args_slice (int len, char ** argv) {
    _yrt_slice_t arr;
    arr.data = GC_malloc (sizeof (_yrt_slice_t) * len);
    arr.len = (unsigned long) len;

    for (int i = 0 ; i < len ; i++) {
        _yrt_slice_t inner;
        inner.data = argv[i];
        inner.len = strlen (argv [i]);
        ((_yrt_slice_t*) arr.data)[i] = inner;
    }

    __MAIN_ARGS__ = arr;    
    return arr;
}

_yrt_slice_t _yrt_get_main_args () {
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
    
    installHandler ();
    _yrt_exc_init ();
            
    int ret = y_main (_yrt_create_args_slice (argc, argv));
    return ret;
}

int _yrt_run_main (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 0;

    GC_INIT ();
    
    installHandler ();
    _yrt_exc_init ();
    
    
    int ret = y_main (_yrt_create_args_slice (argc, argv));
    return ret;
}

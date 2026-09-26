
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include <gc/gc.h>
#include "../rt/memory/types.h"

void _yrt_init_runtime (int isDebug);
_yrt_slice_t _yrt_create_args_slice (int len, char ** argv);
int _yrt_run_unittests_impl (_yrt_slice_t);
void _yrt_register_unittest_impl (_yrt_slice_t name, void (*ptr) (_yrt_slice_t));

typedef struct { void * closure; uint8_t (*func) (void *, void **); } _yrt_param_gen_t;

void _yrt_register_parameterized_unittest_impl (_yrt_slice_t name,
                                                _yrt_param_gen_t (*provider) (),
                                                void (*ptr) (void *));

void _yrt_unittest_coverage_hit_call (void * caller, void * callee);
void _yrt_unittest_coverage_hit_enter (void * func);

// Build the GC-allocated, '\0'-terminated name slice a registration entry point is given.
static _yrt_slice_t test_name_slice (char * func) {
  _yrt_slice_t arr;
  arr.len = strlen (func) + 1;
  arr.data = GC_malloc (arr.len);
  memcpy (arr.data, func, arr.len - 1);

  return arr;
}

void _yrt_register_unittest (char * func, void (*ptr) (_yrt_slice_t)) {
  _yrt_register_unittest_impl (test_name_slice (func), ptr);
}

// Register a parameterized __test: `provider` returns a generator yielding a pointer to each of
// its parameter sets, and `ptr` is called once per yielded pointer. `provider` runs from the
// launcher, not here: the runtime is not initialized yet (cf. _yrt_run_unittests).
void _yrt_register_parameterized_unittest (char * func,
                                           _yrt_param_gen_t (*provider) (),
                                           void (*ptr) (void *)) {
  _yrt_register_parameterized_unittest_impl (test_name_slice (func), provider, ptr);
}

// Resume the generator (closure, func) a provider returned once, storing the parameter set
// pointer it yields in `out`. Returns 0 once the generator is exhausted.
uint8_t _yrt_next_parameter_set (void * closure, uint8_t (*func) (void *, void **), void ** out) {
  return func (closure, out);
}

int _yrt_run_unittests (int argc, char ** argv) {
  // The generated main of a unittest binary calls this entry point directly,
  // instead of _yrt_run_main[_debug], so the runtime is still uninitialized
  // here. Test binaries are always compiled with debug enabled, hence the
  // isDebug argument, which is what makes stack traces available in tests.
  _yrt_init_runtime (1);

  return _yrt_run_unittests_impl (_yrt_create_args_slice (argc, argv));
}

// The number of columns of the terminal stdout is, 0 if it is not a terminal
uint32_t _yrt_test_terminal_width () {
  struct winsize w;
  if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &w) != 0) return 0;

  return w.ws_col;
}

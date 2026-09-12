
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gc/gc.h>
#include "../rt/memory/types.h"

void _yrt_init_runtime (int isDebug);
_yrt_slice_t _yrt_create_args_slice (int len, char ** argv);
int _yrt_run_unittests_impl (_yrt_slice_t);
void _yrt_register_unittest_impl (_yrt_slice_t name, void (*ptr) (_yrt_slice_t));
void _yrt_register_parameterized_unittest_impl (_yrt_slice_t name,
                                                _yrt_slice_t (*provider) (),
                                                void (*ptr) (_yrt_slice_t, uint64_t));

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

// Register a parameterized __test: `provider` returns its parameter sets as a slice, and `ptr`
// is called once per element with that slice and the index of the element. `provider` runs from
// the launcher, not here: the runtime is not initialized yet (cf. _yrt_run_unittests).
void _yrt_register_parameterized_unittest (char * func,
                                           _yrt_slice_t (*provider) (),
                                           void (*ptr) (_yrt_slice_t, uint64_t)) {
  _yrt_register_parameterized_unittest_impl (test_name_slice (func), provider, ptr);
}

int _yrt_run_unittests (int argc, char ** argv) {
  // The generated main of a unittest binary calls this entry point directly,
  // instead of _yrt_run_main[_debug], so the runtime is still uninitialized
  // here. Test binaries are always compiled with debug enabled, hence the
  // isDebug argument, which is what makes stack traces available in tests.
  _yrt_init_runtime (1);

  return _yrt_run_unittests_impl (_yrt_create_args_slice (argc, argv));
}



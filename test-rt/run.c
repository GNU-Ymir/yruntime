
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gc/gc.h>
#include "../rt/memory/types.h"


_yrt_slice_ _yrt_create_args_slice (int len, char ** argv);
int _yrt_run_unittests_impl ();
void _yrt_register_unittest_impl (_yrt_slice_ name, void (*ptr) ());

void _yrt_register_unittest (char * func, void (*ptr) ()) {
  _yrt_slice_ arr;
  arr.len = strlen (func) + 1;
  arr.data = GC_malloc (arr.len);
  memcpy (arr.data, func, arr.len - 1);

  _yrt_register_unittest_impl (arr, ptr);
}

int _yrt_run_unittests (int argc, char ** argv) {
  return _yrt_run_unittests_impl (_yrt_create_args_slice (argc, argv));
}

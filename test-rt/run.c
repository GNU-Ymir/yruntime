
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gc/gc.h>
#include "../c/yarray.h"


_yrt_array_ _yrt_create_args_array (int len, char ** argv);/*  { */
/*     _yrt_array_ arr; */
/*     arr.data = GC_malloc (sizeof (_yrt_array_) * len); */
/*     arr.len = (unsigned long) len; */

/*     for (int i = 0 ; i < len ; i++) { */
/*         _yrt_array_ inner; */
/*         inner.data = argv[i]; */
/*         inner.len = strlen (argv [i]); */
/*         ((_yrt_array_*) arr.data)[i] = inner; */
/*     } */

/*     /\* __MAIN_ARGS__ = arr; *\/ */
/*     return arr; */
/* } */

int _yrt_run_unittests_impl ();

void _yrt_register_unittest (char * func, void (*ptr) ()) {
  printf ("%s\n", func);
}

int _yrt_run_unittests (int argc, char ** argv) {
  return _yrt_run_unittests_impl (_yrt_create_args_array (argc, argv));
}

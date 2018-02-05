#include <array.hh>
#include <string.h>
#include <stdlib.h>

extern "C" int y_run_main (int argc, char ** argv, int(*y_main) (Array)) { 
    auto args = (Array*) malloc (sizeof (Array) * argc);
    for (int i = 0 ; i < argc ; i++) {
	args [i] = {strlen (argv [i]), argv [i]};
    }
    auto ret = y_main ({(unsigned long) argc, args});
    free (args);
    return ret;
}

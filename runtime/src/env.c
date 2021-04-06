#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <yarray.h>
#include <ymemory.h>

_yrt_array_ _yrt_get_current_dir () {
    char cwd [PATH_MAX];
    _yrt_array_ arr;    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
	arr.data = cwd;
	arr.len = strlen (cwd);
	arr = _yrt_dup_slice (arr, sizeof (char));
   } else {
	arr.data = NULL;
	arr.len = 0;
   }
    return arr;
}

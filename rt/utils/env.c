#include <rt/utils/env.h>

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include <rt/memory/alloc.h>

_yrt_slice_t _yrt_get_current_dir () {
    char cwd [PATH_MAX];
    _yrt_slice_t result;
    memset (&result, 0, sizeof (_yrt_slice_t));

    if (getcwd (cwd, sizeof(cwd)) != NULL) {
        _yrt_alloc_slice_no_set (&result, strlen (cwd), 1);
        memcpy (result.data, cwd, result.len);
        return result;

    }

    return result;
}

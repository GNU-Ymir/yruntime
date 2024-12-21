#include <errno.h>
#include <string.h>
#include "errno.h"
#include <rt/utils/gc.h>

int _yrt_get_errno () {
    return errno;
}

void _yrt_set_errno (int err) {
    errno = err;
}

_yrt_slice_t _yrt_str_get_errno (int err) {
    char * errstr = strerror (err);
    uint64_t len = strlen (errstr);
    char * result = (char*) (GC_malloc (len + 1));

    memcpy (result, errstr, len);
    result [len] = 0;

    _yrt_slice_t slc;
    slc.len = len;
    slc.data = result;
    slc.blk_info = NULL;

    return slc;
}

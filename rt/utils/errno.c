#include <errno.h>

int _yrt_get_errno () {
    return errno;
}

void _yrt_set_errno (int err) {
    errno = err;
}

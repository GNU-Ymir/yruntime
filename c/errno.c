#include <errno.h>

int _yrt_get_errno_ () {
    return errno;
}

void _yrt_set_errno_ (int err) {
    errno = err;
}

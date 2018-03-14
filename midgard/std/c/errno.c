#include <errno.h>

int __get_errno__ () {
    return errno;
}

void __set_errno__ (int err) {
    errno = err;
}

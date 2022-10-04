#ifdef __linux__

#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

void _yrt_fd_set (int fd, fd_set * set) {
    FD_SET(fd, set);
}

void _yrt_fd_zero (fd_set * set) {
    FD_ZERO(set);
}


int _yrt_fd_isset (int fd, fd_set * set) {
    return FD_ISSET(fd, set);
}

#endif

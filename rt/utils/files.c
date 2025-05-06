#include <rt/utils/files.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
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

char _yrt_file_date (char * path, int64_t * sec, uint64_t * nsec) {
    struct stat st;
    if (lstat (path, &st) != 0) return 0;

    sec [0] = st.st_mtime;
    nsec [0] = 0;
    // nsec [0] = st.st_mtime_nsec;
    return 1;
}

char _yrt_is_file (char * path, char followLink) {
    struct stat st;
    if (followLink) {
        if (stat (path, &st) != 0) return 0;
    } else {
        if (lstat (path, &st) != 0) return 0;
    }

    return (st.st_mode & S_IFMT) == S_IFREG;
}

char _yrt_is_link (char * path) {
    struct stat st;
    if (lstat (path, &st) != 0) return 0;

    return (st.st_mode & S_IFMT) == S_IFLNK;
}

char _yrt_is_dir (char * path, char followLink) {
    struct stat st;
    if (followLink) {
        if (stat (path, &st) != 0) return 0;
    } else {
        if (lstat (path, &st) != 0) return 0;
    }

    return (st.st_mode & S_IFMT) == S_IFDIR;
}

char _yrt_is_executable (char * path) {
    return access (path, X_OK) == 0;
}

char _yrt_is_writable (char * path) {
    return access (path, W_OK) == 0;
}

char _yrt_is_readable (char * path) {
    return access (path, R_OK) == 0;
}

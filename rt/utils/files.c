#include <rt/utils/files.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rt/memory/alloc.h>

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

char _yrt_exists (char * path, char followLink) {
    struct stat st;
    if (followLink) return stat (path, &st) == 0;
    return lstat (path, &st) == 0;
}

int _yrt_file_size (char * path, uint64_t * size) {
    struct stat st;
    if (stat (path, &st) != 0) return -1;
    if (!S_ISREG (st.st_mode)) return 0;

    size [0] = st.st_size;
    return 1;
}

char _yrt_file_mode (char * path, char followLink, uint32_t * mode) {
    struct stat st;
    if (followLink) {
        if (stat (path, &st) != 0) return 0;
    } else {
        if (lstat (path, &st) != 0) return 0;
    }

    mode [0] = st.st_mode & 07777;
    return 1;
}

char _yrt_create_file (char * path, int32_t mode) {
    int fd = open (path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
    if (fd < 0) return 0;

    close (fd);
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

_yrt_slice_t _yrt_read_link (char * path) {
    _yrt_slice_t result;
    memset (&result, 0, sizeof (_yrt_slice_t));

    for (size_t size = 256; ; size *= 2) {
        char * buf = malloc (size);
        if (buf == NULL) return result;

        ssize_t len = readlink (path, buf, size);
        if (len >= 0 && (size_t) len < size) {
            _yrt_alloc_slice_no_set (&result, len, 1);
            memcpy (result.data, buf, len);
        }

        free (buf);
        if (len < 0 || (size_t) len < size) return result;
    }
}

_yrt_slice_t _yrt_real_path (char * path) {
    _yrt_slice_t result;
    memset (&result, 0, sizeof (_yrt_slice_t));

    char * resolved = realpath (path, NULL);
    if (resolved == NULL) return result;

    _yrt_alloc_slice_no_set (&result, strlen (resolved), 1);
    memcpy (result.data, resolved, result.len);
    free (resolved);

    return result;
}

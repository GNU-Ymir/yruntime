#define _GNU_SOURCE
#include <rt/memory/capture.h>
#include <rt/memory/alloc.h>

#include <stdlib.h>
#include <string.h>

/**
 * An in-memory stream receiving the output of the threads that share it. It is freed once the
 * thread that began it and every thread that adopted it released it, whichever comes last.
 * */
typedef struct _yrt_i_capture_t {
    FILE * file;
    char * data;
    size_t len;
    size_t cap;

    // set by _yrt_capture_end, the writes that come after it are forwarded to stdout
    int detached;
    int refs;

    // the capture that was active in the owning thread when this one began
    struct _yrt_i_capture_t * outer;
} _yrt_i_capture_t;

static __thread _yrt_i_capture_t * __CAPTURE__ = NULL;
static __thread _yrt_i_capture_t * __ADOPTED__ = NULL;

// Called by stdio with the lock of the stream held, the stream being unbuffered
static ssize_t _capture_write (void * cookie, const char * buf, size_t size) {
    _yrt_i_capture_t * c = (_yrt_i_capture_t*) cookie;
    if (c-> detached) {
        return fwrite (buf, 1, size, stdout);
    }

    if (c-> len + size > c-> cap) {
        size_t cap = c-> cap == 0 ? 1024 : c-> cap;
        while (c-> len + size > cap) cap *= 2;

        char * data = realloc (c-> data, cap);
        if (data == NULL) return 0;

        c-> data = data;
        c-> cap = cap;
    }

    memcpy (c-> data + c-> len, buf, size);
    c-> len += size;

    return size;
}

void _yrt_i_capture_release (void * capture) {
    _yrt_i_capture_t * c = (_yrt_i_capture_t*) capture;
    if (c != NULL && __atomic_sub_fetch (&c-> refs, 1, __ATOMIC_ACQ_REL) == 0) {
        fclose (c-> file);
        free (c-> data);
        free (c);
    }
}

FILE * _yrt_stdout () {
    return __CAPTURE__ != NULL ? __CAPTURE__-> file : stdout;
}

FILE * _yrt_stderr () {
    return __CAPTURE__ != NULL ? __CAPTURE__-> file : stderr;
}

void _yrt_capture_begin () {
    _yrt_i_capture_t * c = calloc (1, sizeof (_yrt_i_capture_t));
    cookie_io_functions_t funcs = { .read = NULL, .write = &_capture_write, .seek = NULL, .close = NULL };

    c-> file = fopencookie (c, "w", funcs);
    if (c-> file == NULL) {
        free (c);
        return;
    }

    setvbuf (c-> file, NULL, _IONBF, 0);
    c-> refs = 1;
    c-> outer = __CAPTURE__;
    __CAPTURE__ = c;
}

_yrt_slice_t _yrt_capture_end () {
    _yrt_slice_t result = { .len = 0, .data = NULL, .blk_info = NULL };
    _yrt_i_capture_t * c = __CAPTURE__;
    if (c == NULL || c == __ADOPTED__) return result;

    flockfile (c-> file);
    if (c-> len != 0) {
        _yrt_alloc_slice_no_set (&result, c-> len, 1);
        memcpy (result.data, c-> data, c-> len);
    }

    c-> detached = 1;
    funlockfile (c-> file);

    __CAPTURE__ = c-> outer;
    _yrt_i_capture_release (c);

    return result;
}

void * _yrt_i_capture_retain () {
    _yrt_i_capture_t * c = __CAPTURE__;
    if (c != NULL) {
        __atomic_add_fetch (&c-> refs, 1, __ATOMIC_ACQ_REL);
    }

    return c;
}

void _yrt_i_capture_adopt (void * capture) {
    __ADOPTED__ = (_yrt_i_capture_t*) capture;
    __CAPTURE__ = __ADOPTED__;
}

void _yrt_i_capture_drop () {
    _yrt_i_capture_release (__ADOPTED__);
    __ADOPTED__ = NULL;

    __CAPTURE__ = NULL;
}

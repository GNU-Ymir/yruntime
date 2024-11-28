#include <rt/utils/string.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <rt/utils/gc.h>
#include <rt/memory/alloc.h>

_yrt_slice_t str_from_int (int32_t value) {
    unsigned int len = snprintf (NULL, 0, "%d", value);
    char * alloc = malloc (len);
    sprintf (alloc, "%d", value);

    _yrt_slice_t result;
    result = str_create_len (alloc, len);
    free (alloc);

    return result;
}

_yrt_slice_t str_from_char (char value) {
    unsigned int len = snprintf (NULL, 0, "%c", value);
    char * alloc = malloc (len);
    sprintf (alloc, "%c", value);

    _yrt_slice_t result;
    result = str_create_len (alloc, len);
    free (alloc);

    return result;
}

_yrt_slice_t str_from_ptr (void* value) {
    unsigned int len = snprintf (NULL, 0, "%p", value);
    char * alloc = malloc (len);
    sprintf (alloc, "%p", value);

    _yrt_slice_t result;
    result = str_create_len (alloc, len);
    free (alloc);

    return result;
}

_yrt_slice_t str_create (const char * data) {
    return str_create_len (data, strlen (data));
}

_yrt_slice_t str_create_len (const char * data, uint64_t len) {
    _yrt_slice_t ret;
    memset (&ret, 0, sizeof (_yrt_slice_t));

    ret.data = (char*) data;
    ret.len = len;

    return ret;
}

_yrt_slice_t str_copy_len (const char *data, uint64_t len) {
    _yrt_slice_t result;
    _yrt_alloc_slice_no_set (&result, len, 1);
    memcpy (result.data, data, len);

    return result;
}

_yrt_slice_t str_copy (const char * data) {
    return str_copy_len (data, strlen (data));
}

_yrt_slice_t str_empty () {
    _yrt_slice_t str;
    memset (&str, 0, sizeof (_yrt_slice_t));

    return str;
}

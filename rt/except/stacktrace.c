#define _GNU_SOURCE
#include <rt/except/stacktrace.h>

#include <rt/utils/demangle.h>
#include <rt/memory/alloc.h>

#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gc/gc.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <unistd.h>     /* For close() */

int __YRT_MAXIMUM_TRACE_LEN__ = 128;

static pthread_mutex_t __YRT_STACK_TRACE_MUTEX__ = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#include <execinfo.h>
#include <unistd.h>

#define PATH_MAX 255

char* _yrt_i_resolve_path (const char * filename, char * resolved, int size) {
    int len_f = strlen (filename);
    if (access (filename, F_OK) == 0) {
        memcpy (resolved, filename, len_f);
        resolved [len_f] = 0;
        return resolved;
    }

    char * PATH_AUX = getenv ("PATH");
    if (PATH_AUX == NULL) return NULL;
    char * PATH = strdup (PATH_AUX);

    char * savePtr = NULL;
    char * strToken = strtok_r (PATH, ":", &savePtr);
    int found = 0;
    while (strToken != NULL) {
        if (found == 0) {
            int len = strlen (strToken);
            if (len + len_f + 2 < PATH_MAX) {
                int n = snprintf (resolved, PATH_MAX, "%s/%s", strToken, filename);
                resolved [n] = '\0';

                if (access (resolved, F_OK) == 0) {
                    found = 1;
                    break;
                }
            }
        }
        strToken = strtok_r (NULL, ":", &savePtr);
    }

    free (PATH);
    if (found) return resolved;
    else return NULL;
}

_yrt_slice_t _yrt_exc_resolve_stack_trace (_yrt_slice_t syms) {
	_yrt_slice_t result;
	memset (&result, 0, sizeof (result));
    if (__YRT_DEBUG__ != 1 && __YRT_FORCE_DEBUG__ != 1) return result;

    pthread_mutex_lock (&__YRT_STACK_TRACE_MUTEX__);
    char** messages = NULL;
    messages = backtrace_symbols (syms.data, (uint32_t) syms.len);
    result = _yrt_i_str_create ("╭  Stack trace :");

    for (uint32_t i = 2 ; i < syms.len - 5 ; i++) {
        size_t p = 0;
        char filename [245];
        while(messages[i][p] != '(' && messages[i][p] != 0) {
            filename [p] = messages [i][p];
            p += 1;
        }
        filename [p] = '\0';

        char resolved [PATH_MAX];
        char* succ = _yrt_i_resolve_path (filename, resolved, PATH_MAX);

        void * sym = ((void**) syms.data)[i];
        struct _yrt_reflect_symbol_t ref_sym;
        struct _yrt_reflect_debug_symbol_info_t debugInfo;
        memset (&debugInfo.file, 0, sizeof (debugInfo.file));

        if (succ != NULL) {
            _yrt_slice_t resolvedC8 = _yrt_i_str_create (resolved);
            ref_sym = _yrt_reflect_find_function_from_addr_with_elf_name (sym, resolvedC8);
            debugInfo = _yrt_reflect_get_debug_info (resolvedC8, sym, ref_sym);
        }

        if (ref_sym.type == FUNCTION) {
            _yrt_slice_t tmp;
            if (debugInfo.file.len != 0) {
                tmp = _yrt_i_str_create ("\n╞═ bt ╕ #");
            } else {
                tmp = _yrt_i_str_create ("\n╞═ bt ═ #");
            }

            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = _yrt_i_str_from_int (i - 1);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));

            int need_break = 0;
            if (ref_sym.name.data != NULL) {
                _yrt_slice_t name = _yrt_i_demangle_symbol (ref_sym.name.data, ref_sym.name.len);

                tmp = _yrt_i_str_create (" in function \e[33m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                _yrt_append_slice (&result, &name, sizeof (uint8_t));
                tmp = _yrt_i_str_create ("\e[0m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                need_break = (strcmp (name.data, "main (...)") == 0);
            }

            if (debugInfo.file.len != 0) {
                tmp = _yrt_i_str_create ("\n│     ╘═> \e[32m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                _yrt_append_slice (&result, &debugInfo.file, sizeof (uint8_t));
                tmp = _yrt_i_str_create ("\e[0m:");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                tmp = _yrt_i_str_from_int (debugInfo.line);
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            }

            if (need_break) break;
        } else {
            _yrt_slice_t tmp = _yrt_i_str_create ("\n╞═ bt ╕ #");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = _yrt_i_str_from_int (i - 1);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = _yrt_i_str_create (" in ??\n│     ╘═> \e[32m");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = _yrt_i_str_create (filename);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = _yrt_i_str_create ("\e[0m");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
        }
    }

    _yrt_slice_t tmp = _yrt_i_str_create ("\n╰\0");
    _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
    _yrt_reflect_clear_debug_info ();
    free (messages);
    pthread_mutex_unlock (&__YRT_STACK_TRACE_MUTEX__);

	return result;
}

_yrt_slice_t _yrt_exc_get_stack_trace () {
	_yrt_slice_t result;
	memset (&result, 0, sizeof (_yrt_slice_t));
    if (__YRT_DEBUG__ != 1 && __YRT_FORCE_DEBUG__ != 1) return result;

    void **trace = (void**) malloc (__YRT_MAXIMUM_TRACE_LEN__ * sizeof (void*));

    int trace_size = backtrace (trace, __YRT_MAXIMUM_TRACE_LEN__);

    void** res = GC_malloc (trace_size * sizeof (void*));
    memcpy (res, trace, sizeof (void*) * trace_size);
    free (trace);

    result.len = trace_size;
    result.data = res;

	return result;
}

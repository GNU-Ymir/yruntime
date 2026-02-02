#include <rt/except/stacktrace.h>

#include <rt/utils/demangle.h>
#include <rt/memory/alloc.h>

#include <getopt.h>
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

#include <execinfo.h>
#include <unistd.h>

#define PATH_MAX 255

char* _yrt_resolve_path (const char * filename, char * resolved, int size) {
    int len_f = strlen (filename);
    if (access (filename, F_OK) == 0) {
        memcpy (resolved, filename, len_f);
        resolved [len_f] = 0;
        return resolved;
    }

    char * PATH_AUX = getenv ("PATH");
    char * PATH = malloc (strlen (PATH_AUX));
    memcpy (PATH, PATH_AUX, strlen (PATH_AUX));

    char * strToken = strtok (PATH, ":");
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
        strToken = strtok (NULL, ":");
    }

    free (PATH);
    if (found) return resolved;
    else return NULL;
}

_yrt_slice_t _yrt_exc_resolve_stack_trace (_yrt_slice_t syms) {
	_yrt_slice_t result;
	memset (&result, 0, sizeof (result));
    if (__YRT_DEBUG__ != 1 && __YRT_FORCE_DEBUG__ != 1) return result;

    char** messages = NULL;
    messages = backtrace_symbols (syms.data, (uint32_t) syms.len);
    result = str_create ("╭  Stack trace :");

    for (uint32_t i = 2 ; i < syms.len - 5 ; i++) {
        size_t p = 0;
        char filename [245];
        while(messages[i][p] != '(' && messages[i][p] != 0) {
            filename [p] = messages [i][p];
            p += 1;
        }
        filename [p] = '\0';

        char resolved [PATH_MAX];
        char* succ = _yrt_resolve_path (filename, resolved, PATH_MAX);

        void * sym = ((void**) syms.data)[i];
        struct _yrt_reflect_symbol_t ref_sym;
        struct _yrt_reflect_debug_symbol_info_t debugInfo;
        memset (&debugInfo.file, 0, sizeof (debugInfo.file));

        if (succ != NULL) {
            _yrt_slice_t resolvedC8 = str_create (resolved);
            ref_sym = _yrt_reflect_find_function_from_addr_with_elf_name (sym, resolvedC8);
            debugInfo = _yrt_reflect_get_debug_info (resolvedC8, sym, ref_sym);
        }

        if (ref_sym.type == FUNCTION) {
            _yrt_slice_t tmp;
            if (debugInfo.file.len != 0) {
                tmp = str_create ("\n╞═ bt ╕ #");
            } else {
                tmp = str_create ("\n╞═ bt ═ #");
            }

            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = str_from_int (i - 1);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));

            int need_break = 0;
            if (ref_sym.name.data != NULL) {
                _yrt_slice_t name = _yrt_demangle_symbol (ref_sym.name.data, ref_sym.name.len);

                tmp = str_create (" in function \e[33m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                _yrt_append_slice (&result, &name, sizeof (uint8_t));
                tmp = str_create ("\e[0m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                need_break = (strcmp (name.data, "main (...)") == 0);
            }

            if (debugInfo.file.len != 0) {
                tmp = str_create ("\n│     ╘═> \e[32m");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                _yrt_append_slice (&result, &debugInfo.file, sizeof (uint8_t));
                tmp = str_create ("\e[0m:");
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
                tmp = str_from_int (debugInfo.line);
                _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            }

            if (need_break) break;
        } else {
            _yrt_slice_t tmp = str_create ("\n╞═ bt ╕ #");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = str_from_int (i - 1);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = str_create (" in ??\n│     ╘═> \e[32m");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = str_create (filename);
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
            tmp = str_create ("\e[0m");
            _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
        }
    }

    _yrt_slice_t tmp = str_create ("\n╰\0");
    _yrt_append_slice (&result, &tmp, sizeof (uint8_t));
    _yrt_reflect_clear_debug_info ();

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

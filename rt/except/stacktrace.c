#include <rt/except/stacktrace.h>

#include <rt/utils/demangle.h>

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

int _yrt_find_dwarf_die (Dwarf_Debug obj, uint64_t addr, Dwarf_Die * die) {
    Dwarf_Die returnDie;
    Dwarf_Error error = DW_DLE_NE;
    Dwarf_Arange * aranges;
    Dwarf_Signed arange_count;

    Dwarf_Bool found = 0;
    if (dwarf_get_aranges (obj, &aranges, &arange_count, &error) != DW_DLV_OK) {
        aranges = NULL;
    }

    if (aranges) {
        Dwarf_Arange arange;
        if (dwarf_get_arange (aranges, arange_count, addr, &arange, &error) == DW_DLV_OK) {

            Dwarf_Off cu_die_offset;
            if (dwarf_get_cu_die_offset (arange, &cu_die_offset, &error) == DW_DLV_OK) {
                int dwarf_res = dwarf_offdie_b (obj, cu_die_offset, 1, &returnDie, &error);

                found = (dwarf_res == DW_DLV_OK);
            }
            dwarf_dealloc (obj, arange, DW_DLA_ARANGE);
        }
    }

    if (found) {
        *die = returnDie;
        return 0;
    }

    return 1;
}

void _yrt_find_in_dwarf (const char * filename, void * addr, _yrt_slice_t * file, int * line) {
    Dwarf_Debug dbg = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Error error;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;

    int fd = open(filename, O_RDONLY);
    if (fd == 0)  return;

    if (dwarf_init (fd, DW_DLC_READ, NULL, NULL, &dbg, &error)) {
        goto end;
    }


    Dwarf_Die die;
    if (_yrt_find_dwarf_die (dbg, (uint64_t) addr, &die)) {
        goto end_dwarf;
    }

    Dwarf_Line * lines;
    Dwarf_Signed nlines;
    if (dwarf_srclines (die, &lines, &nlines, &error)) {
        goto end_die;
    }

    char * srcfile;
    Dwarf_Unsigned lineno;
    Dwarf_Addr lineaddr;
    int n;
    uint64_t max = (uint64_t) addr;
    for (n = 0; n < nlines; n++) {
        /* Retrieve the virtual address for this line. */
        if (dwarf_lineaddr(lines[n], &lineaddr, &error)) {
            break;
        }

        int dist = abs ((uint64_t) addr) - lineaddr;
        if (dist < max) {
            max = dist;
            /* Retrieve the file name for this errorscriptor. */
            if (dwarf_linesrc(lines[n], &srcfile, &error)) {
                break;
            }

            *file = str_copy (srcfile);
            dwarf_dealloc (dbg, srcfile, DW_DLA_STRING);


            /* Retrieve the line number in the source file. */
            if (dwarf_lineno(lines[n], &lineno, &error)) {
                break;
            }

            *line = lineno;
            if (dist == 0) break;
        }
    }

    dwarf_srclines_dealloc (dbg, lines, nlines);

end_die:
    dwarf_dealloc (dbg, die, DW_DLA_DIE);

end_dwarf:
    res = dwarf_finish (dbg, &error);

end:
    close(fd);
}

int _yrt_resolve_address (const char * filename, void* addr, _yrt_slice_t * file, int* line) {
    *file = str_empty ();
    *line = 0;

    _yrt_find_in_dwarf (filename, addr, file, line);
    return 0;
}

_yrt_slice_t _yrt_exc_resolve_stack_trace (_yrt_slice_t syms) {
	_yrt_slice_t result;
	memset (&result, 0, sizeof (result));

    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
    }

	return result;
}

_yrt_slice_t _yrt_exc_get_stack_trace () {
	_yrt_slice_t result;
	memset (&result, 0, sizeof (_yrt_slice_t));

    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
        void **trace = (void**) malloc (__YRT_MAXIMUM_TRACE_LEN__ * sizeof (void*));

        int trace_size = backtrace (trace, __YRT_MAXIMUM_TRACE_LEN__);

        void** res = GC_malloc (trace_size * sizeof (void*));
        memcpy (res, trace, sizeof (void*) * trace_size);
        free (trace);

        _yrt_slice_t arr;
        arr.len = trace_size;
        arr.data = res;
    }

	return result;
}


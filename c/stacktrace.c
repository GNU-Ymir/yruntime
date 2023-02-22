#include "stacktrace.h"
#include "demangle.h"
#include "reflect.h"

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

#ifdef __linux__
#include <execinfo.h>
#include <unistd.h>

#define PATH_MAX 255

char* _yrt_resolve_path (const char * filename, char * resolved, int size) {
    char * PATH_AUX = getenv ("PATH");
    char * PATH = malloc (strlen (PATH_AUX));
    memcpy (PATH, PATH_AUX, strlen (PATH_AUX));
    
    int len_f = strlen (filename);
    
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
    return resolved;
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

void _yrt_find_in_dwarf (const char * filename, void * addr, _ystring * file, int * line) {
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

int _yrt_resolve_address (const char * filename, void* addr, _ystring * file, int* line) {
    *file = str_empty ();
    *line = 0;
    
    _yrt_find_in_dwarf (filename, addr, file, line);		        
    return 0;
}

_yrt_array_ _yrt_exc_resolve_stack_trace (_yrt_array_ syms) {
    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {	
	char **messages = (char **)NULL;
	messages = backtrace_symbols(syms.data, (int) syms.len);
	/* skip first stack frame (points here) */
	_ystring ret = str_empty ();
	ret = str_concat_c_str (ret, "╭  Stack trace :");
	
	for (int i=2; i < syms.len; ++i)
	{    	    
	    size_t p = 0;
	    char filename [245];
	    while(messages[i][p] != '(' && messages[i][p] != 0) {
		filename [p] = messages [i][p];
		p += 1;
	    }
	    filename [p] = '\0';

	    char resolved [PATH_MAX];
	    char* succ = _yrt_resolve_path (filename, resolved, PATH_MAX);

	    _ystring file;
	    _ystring func;
	    int line;
	    _yrt_c8_array_ resolvedC8 = {.len = strlen (resolved), .data = resolved };
	    
	    struct ReflectSymbol sym = _yrt_reflect_find_symbol_from_addr_with_elf_name (((void**) syms.data) [i], resolvedC8);
	    	    
	    if (sym.name != NULL) {
		_yrt_resolve_address (resolved, ((void**) syms.data) [i], &file, &line);	    
		func = str_create (sym.name);
		
		if (file.data != NULL) {
		    ret = str_concat_c_str (ret, "\n╞═ bt ╕ #");
		    ret = str_concat (ret, str_from_int (i - 1));
		} else {
		    ret = str_concat_c_str (ret, "\n╞═ bt ═ #");
		    ret = str_concat (ret, str_from_int (i - 1));
		}

		int need_break = 0;
		if (func.data != NULL) {
		    _ystring name = _yrt_demangle_symbol (func.data, func.len);
		    ret = str_concat_c_str (ret, " in function \e[33m");
		    ret = str_concat (ret, name);
		    ret = str_concat_c_str (ret, "\e[0m");
		    need_break = (strcmp (name.data, "main (...)") == 0); 
		} 
		
		if (file.data != NULL) {		    
		    ret = str_concat_c_str (ret, "\n│     ╘═> \e[32m");
		    ret = str_concat_c_str (ret, file.data);
		    ret = str_concat_c_str (ret, "\e[0m:");
		    ret = str_concat (ret, str_from_int (line));
		}
		if (need_break) break;
	    } else {
		ret = str_concat_c_str (ret, "\n╞═ bt ╕ #");
		ret = str_concat (ret, str_from_int (i - 1));
		ret = str_concat_c_str (ret, " in ??\n│     ╘═> \e[32m");
		ret = str_concat_c_str (ret, filename);
		ret = str_concat_c_str (ret, "\e[0m");
	    }
	    
	}
	ret = str_concat_c_str (ret, "\n╰\0");
	ret = str_fit (ret);
	_yrt_array_ arr;
	arr.len = ret.len;
	arr.data = ret.data;
    
	return arr;
    } else {
	_yrt_array_ arr;
	arr.len = 0;
	arr.data = NULL;
	return arr;
    }
}

_yrt_array_ _yrt_exc_get_stack_trace () {
    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
	void **trace = (void**) malloc (__YRT_MAXIMUM_TRACE_LEN__ * sizeof (void*));

	int trace_size = backtrace(trace, __YRT_MAXIMUM_TRACE_LEN__);	

	void** res = GC_malloc (trace_size * sizeof (void*));
	memcpy (res, trace, sizeof (void*) * trace_size);
	free (trace);
	
	_yrt_array_ arr;
	arr.len = trace_size;
	arr.data = res;
	
	return arr;	
    } else {
	_yrt_array_ arr;
	arr.len = 0;
	arr.data = NULL;
	return arr;
    }
}

#elif _WIN32

#include <Windows.h>
#include "dbghelp.h"
#include <windows.h>


void* getCallerAddr (int i) {
    switch (i) {
    case 0 :
	return __builtin_frame_address (2);
    case 1 :
	return __builtin_frame_address (3);
    case 2 :
	return __builtin_frame_address (4);
    default:
	return NULL;
    }
}

void** captureBackTrace (int i, int max, void** buffer) {
    if (i == max) return buffer;
    buffer[i] = getCallerAddr (i);
    return captureBackTrace (i + 1, max, buffer);
}

_yrt_array_ _yrt_exc_resolve_stack_trace (_yrt_array_ syms) {
    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
	_ystring ret = str_empty ();
	ret = str_concat_c_str (ret, "[  Stack trace :");
	
	for (int i = 2 ; i < syms.len ; i++) {
	    ret = str_concat_c_str (ret, "\n|= bt =| #");
	    ret = str_concat (ret, str_from_int (i - 1));
	    
	    void* ptr = ((void**) syms.data)[i];
	    struct ReflectSymbol sym = _yrt_reflect_find_symbol_from_addr (ptr);
	    
	    if (sym.ptr != NULL) {
		_ystring name = _yrt_demangle_symbol (sym.name, strlen (sym.name));
		ret = str_concat_c_str (ret, " in function \e[33m");
		ret = str_concat (ret, name);
		ret = str_concat_c_str (ret, "\e[0m");
		if (sym.locFile != NULL) {
		    ret = str_concat_c_str (ret, "\n|     |=> \e[32m");
		    ret = str_concat_c_str (ret, sym.locFile);
		    ret = str_concat_c_str (ret, "\e[0m:");
		    ret = str_concat (ret, str_from_int (sym.locLine));
		}
		
		if (strcmp (name.data, "main (...)") == 0) break;
	    } else {
		ret = str_concat_c_str (ret, " in function \e[33m:??\e[0m");
	    }
	}

	ret = str_concat_c_str (ret, "\n[\0");
	ret = str_fit (ret);
	_yrt_array_ arr;
	arr.len = ret.len;
	arr.data = ret.data;
    
	return arr;
    } else {
	_yrt_array_ arr;
	arr.len = 0;
	arr.data = NULL;
    
	return arr;
    }
}

_yrt_array_ _yrt_exc_get_stack_trace () {
    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
	void** trace = (void**) malloc (__YRT_MAXIMUM_TRACE_LEN__ * sizeof (void*));
	int trace_size = (int) CaptureStackBackTrace (0, __YRT_MAXIMUM_TRACE_LEN__, trace, NULL);
	
	void** res = GC_malloc (trace_size * sizeof (void*));
	memcpy (res, trace, sizeof(void*) * trace_size);
	free (trace);

	_yrt_array_ arr;
	arr.len = trace_size;
	arr.data = res;

	return arr;
    } else {
	_yrt_array_ arr;
	arr.len = 0;
	arr.data = NULL;

	return arr;
    }
}

#endif

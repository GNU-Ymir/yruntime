#include "stacktrace.h"
#include "demangle.h"

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gc/gc.h>


int __YRT_MAXIMUM_TRACE_LEN__ = 128;

#ifdef __linux__
#include <execinfo.h>
#include <bfd.h>
#include <unistd.h>

#define PATH_MAX 255

char* _yrt_resolve_path (const char * filename, char * resolved, int size) {
    char * PATH_AUX = getenv ("PATH");
    char * PATH = malloc (strlen (PATH_AUX));
    memcpy (PATH, PATH_AUX, strlen (PATH_AUX));

    char * strToken = strtok (PATH, ":");
    int found = 0;
    while (strToken != NULL) {
	if (found == 0) {
	    int len = strlen (strToken);
	    len = len > size ? size : len;
	    int i = 0;
	    for (i = 0 ; i < len ; i ++) {
		resolved [i] = strToken [i];
	    }
	    resolved [i] = '/';
	    for (i = 0 ; i < size - len; i++) {
		resolved [i + len + 1] = filename [i];
		if (filename [i] == '\0') break;
	    }
	
	    if (access (resolved, F_OK) == 0) {
		found = 1;
	    }
	}
	strToken = strtok (NULL, ":");	
    }

    free (PATH);
    if (found == 0) {
	for (int i = 0 ; i < size; i ++) {
	    resolved [i] = filename [i];
	    if (filename [i] == '\0') break;
	}
    }
    return resolved;
}

void _yrt_slurp_sym_table (struct bfd_handle * handle) {
    unsigned int size;
    if ((bfd_get_file_flags (handle-> ptr) & HAS_SYMS) == 0) {
	_yrt_close_bfd_file (*handle);
	handle-> ptr = NULL;
	return;
    }

    handle-> nb_syms = bfd_read_minisymbols (handle-> ptr, FALSE, (void*) &handle-> syms, &size);
    if (handle-> nb_syms == 0)
	handle-> nb_syms = bfd_read_minisymbols (handle-> ptr, TRUE, (void*) &handle-> syms, &size);
    
    if (handle-> nb_syms < 0) {
	_yrt_close_bfd_file (*handle);
	return;
    }
}

struct bfd_handle _yrt_get_bfd_file (const char * filename) {
    char realPath [PATH_MAX + 1];
    struct bfd_handle handle;
    handle.filename = _yrt_resolve_path (filename, realPath, PATH_MAX);
    
    handle.ptr = bfd_openr (handle.filename, NULL);
    if (handle.ptr == NULL)
	return handle;

    if (bfd_check_format (handle.ptr, bfd_object) == 0) {
	_yrt_close_bfd_file (handle);
	handle.ptr = NULL;
	return handle;
    }

    _yrt_slurp_sym_table (&handle);
    return handle;
}

void _yrt_find_address_in_section (bfd * abfd, asection* section, void * data) {
    struct bfd_context * context = (struct bfd_context*) data;    
    if (context-> set != 0) return;
    
    bfd_vma vma;
    bfd_size_type size;

#ifdef bfd_get_section_flags 
    if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
	return;
#else
    if ((bfd_section_flags (section) & SEC_ALLOC) == 0)
	return;
#endif
  
#ifdef bfd_get_section_vma
    vma = bfd_get_section_vma (abfd, section);
#else
    vma = bfd_section_vma (section);
#endif
  
    if (context-> pc < vma)
	return;
#ifdef bfd_get_section_size  
    size = bfd_get_section_size (section);
#else
    size = bfd_section_size (section);
#endif
  
    if (context-> pc >= vma + size)
	return;
    
    context-> set = bfd_find_nearest_line (abfd, section, context-> handle.syms, context-> pc - vma,
					   &context-> file, &context-> function, &context-> line);    
}

int _yrt_resolve_address (const char * filename, void* addr, _ystring * file, _ystring * func, int* line, struct bfd_handle handle) {
    if (handle.ptr != NULL) {

	struct bfd_context context;
	context.handle = handle;
	context.set = 0;
	context.file = NULL;
	context.function = NULL;
	
	context.pc = bfd_scan_vma (str_from_ptr (addr).data, NULL, 16);
	bfd_map_over_sections (handle.ptr, _yrt_find_address_in_section, (void*) &context);
	
	if (context.set != 0) {	    
	    *file = str_create (context.file);
	    *func = str_create (context.function);
	    *line = context.line;
	    return 1;
	} 
    }
    
    *file = str_empty ();
    *func = str_empty ();
    *line = 0;
    return 0;
}

void _yrt_close_bfd_file (struct bfd_handle handle) {
    bfd_close (handle.ptr);
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

	    _ystring file;
	    _ystring func;
	    int line;
	    struct bfd_handle handle = _yrt_get_bfd_file (filename);
	    if (_yrt_resolve_address (filename, ((void**) syms.data) [i], &file, &func, &line, handle)) {
		if (file.data != NULL) {
		    ret = str_concat_c_str (ret, "\n╞═ bt ╕ #");
		    ret = str_concat (ret, str_from_int (i - 1));
		} else {
		    ret = str_concat_c_str (ret, "\n╞═ bt ═ #");
		    ret = str_concat (ret, str_from_int (i - 1));
		}

		if (func.data != NULL) {
		    ret = str_concat_c_str (ret, " in function \e[33m");
		    ret = str_concat (ret, _yrt_demangle_symbol (func.data, func.len));
		    ret = str_concat_c_str (ret, "\e[0m");
		}
		
		if (file.data != NULL) {		    
		    ret = str_concat_c_str (ret, "\n│     ╘═> \e[32m");
		    ret = str_concat_c_str (ret, file.data);
		    ret = str_concat_c_str (ret, "\e[0m:");
		    ret = str_concat (ret, str_from_int (line));
		}
	    } else {
		ret = str_concat_c_str (ret, "\n╞═ bt ╕ #");
		ret = str_concat (ret, str_from_int (i - 1));
		ret = str_concat_c_str (ret, "\n│     ╘═> \e[32m");
		ret = str_concat_c_str (ret, filename);
		ret = str_concat_c_str (ret, "\e[0m:??");
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

#include <windows.h>

_yrt_array_ _yrt_exc_resolve_stack_trace (_yrt_array_ syms) {
    if (__YRT_DEBUG__ == 1 || __YRT_FORCE_DEBUG__ == 1) {
	_ystring ret = str_empty ();
	ret = str_concat_c_str (ret, "[  Stack trace :");
	
	for (int i = 2 ; i < syms.len ; i++) {
	    ret = str_concat_c_str (ret, "\n|= bt =| #");
	    ret = str_concat (ret, str_from_int (i - 1));
	    ret = str_concat_c_str (ret, "\n|     |=> \e[32m");
	    //ret = str_concat_c_str (ret, filename);
	    ret = str_concat_c_str (ret, "\e[0m:??");
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

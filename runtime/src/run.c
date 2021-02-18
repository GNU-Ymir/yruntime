#include "../include/run.h"
#include "../include/ymemory.h"
#include "../include/throw.h"
#include "../include/demangle.h"
#include <gc/gc_disclaim.h>

#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <bfd.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int __YRT_DEBUG__ = 0;

void _yrt_throw_seg_fault ();

void bt_sighandler(int sig, struct sigcontext ctx) {    
    _yrt_throw_seg_fault ();    
}

void installHandler () {
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL); // On seg fault we throw an exception
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
    struct bfd_handle handle;
    handle.filename = filename;
    
    handle.ptr = bfd_openr (filename, NULL);
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

int _yrt_resolve_address (const char * filename, void* addr, _ystring * file, _ystring * func, int* line) {
    struct bfd_handle handle = _yrt_get_bfd_file (filename);
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
    if (__YRT_DEBUG__ == 1) {
	
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
	    if (_yrt_resolve_address (filename, ((void**) syms.data) [i], &file, &func, &line)) {
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
	ret = str_concat_c_str (ret, "\n╰");
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
    if (__YRT_DEBUG__ == 1) {
	void *trace[16];

	int trace_size = backtrace(trace, 16);	

	void** res = GC_malloc (trace_size * sizeof (void*));
	for (int i = 0 ; i< trace_size ; i++) {
	    res [i] = trace [i];
	}
	
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

int _yrt_run_main_debug (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 1;
    bfd_init ();

    installHandler ();
    _yrt_exc_init ();
        
    _yrt_array_ arr;
    arr.data = argv;
    arr.len = (unsigned long) argc;
    
    return y_main (arr);    
}

int _yrt_run_main (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 0;

    installHandler ();
    _yrt_exc_init ();
    
    _yrt_array_ arr;
    arr.data = argv;
    arr.len = (unsigned long) argc;
    
    return y_main (arr);    
}


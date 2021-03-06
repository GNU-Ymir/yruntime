#include "../include/run.h"
#include "../include/ymemory.h"
#include "../include/throw.h"
#include "../include/demangle.h"
#include "../include/reflect.h"
#include <gc/gc_disclaim.h>

#include <limits.h>
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
_yrt_array_ __MAIN_ARGS__;


void _yrt_exit (int i) {
    exit (i);
}

void _yrt_throw_seg_fault ();
void _yrt_exc_panic (char *file, const char *function, unsigned line);

void bt_sighandler(int sig, struct sigcontext ctx) {
    static int second = 0;
    if (second == 1) {
	char * c = NULL;
	*c = 'i';
    }
    second = 1;
    _yrt_throw_seg_fault ();    
}

void installHandler () {
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL); // On seg fault we throw an exception
}

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


void _yrt_exc_panic (char *file, const char *function, unsigned line)
{
        
    fprintf (stderr, "Panic in file \"%s\", at line %u", file, line);
    fprintf (stderr, ", in function \"%s\" !!! \n", function);
    fprintf (stderr, "Please report the error at gnu.ymir@mail.com\n");
    _yrt_array_ trace = _yrt_exc_resolve_stack_trace (_yrt_exc_get_stack_trace ());
    if (trace.len != 0)
    	fprintf (stderr, "%s\n", (char*) trace.data);
    exit (-1);
}


_yrt_array_ _yrt_create_args_array (int len, char ** argv) {
    _yrt_array_ arr;
    arr.data = GC_malloc (sizeof (_yrt_array_) * len);
    arr.len = (unsigned long) len;

    for (int i = 0 ; i < len ; i++) {
	_yrt_array_ inner;
	inner.data = argv[i];
	inner.len = strlen (argv [i]);
	((_yrt_array_*) arr.data)[i] = inner;
    }

    __MAIN_ARGS__ = arr;    
    return arr;
}

_yrt_array_ _yrt_get_main_args () {
    return __MAIN_ARGS__;
}

void _yrt_force_debug () {
    __YRT_DEBUG__ = 1;
    bfd_init ();
}

int _yrt_run_main_debug (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 1;
    
    GC_INIT ();
    bfd_init ();

    installHandler ();
    _yrt_exc_init ();
            
    int ret = y_main (_yrt_create_args_array (argc, argv));
    
    _yrt_elf_clean ();
    return ret;
}

int _yrt_run_main (int argc, char ** argv, int(* y_main)()) {
    __YRT_DEBUG__ = 0;

    GC_INIT ();
    
    installHandler ();
    _yrt_exc_init ();
    
    
    int ret = y_main (_yrt_create_args_array (argc, argv));
    _yrt_elf_clean (); // not sure it is necessary, we are closing the program anyway
    
    return ret;
}


#include <setjmp.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <type.hh>

volatile int _y_exc_handled;

volatile unsigned _y_exc_tries;

char *_y_exc_file;
char *_y_exc_function;
unsigned _y_exc_line;
volatile int _y_exc_code;

TypeInfo* _y_exc_type_info;
void* _y_exc_type_data;

/* This is the global stack of catchers. */
struct _y_exc_stack *_y_exc_global;

/* Stack is actually a linked list of catcher cells. */
struct _y_exc_stack
{
    unsigned num;
    jmp_buf j;
    struct _y_exc_stack *prev;
};


extern "C" void _y_exc_print (FILE *stream, char *file, char *function, unsigned line,
		   int code)
{
    fprintf (stream, "Exception in file \"%s\", at line %u",
	     file, line);
    
    if (function)
	{
	    fprintf (stream, ", in function \"%s\"", function);
	}
    fprintf (stream, ".");
    
#ifdef _Y_EXC_PRINT
    fprintf (stream, " Exception ");
    _Y_EXC_PRINT (code, stream);
#endif
    fprintf (stream, "\n");
}

extern "C" int _y_exc_push (jmp_buf *j, int returned) {
    static _y_exc_stack *head;
    if (returned != 0) { // Le jmp a déjà été déclaré, on est revenu dessus suite à un throw
	return 0;	
    }

    ++ _y_exc_tries;
    /* Using memcpy here is the best alternative. */
    head = (_y_exc_stack*) malloc (sizeof (_y_exc_stack));
    memcpy (&head->j, j, sizeof (jmp_buf));
    head->num = _y_exc_tries;
    head->prev = _y_exc_global;
    _y_exc_global = head;

    return 1;     
}

extern "C" void _y_exc_pop (jmp_buf *j)
{
    struct _y_exc_stack *stored = _y_exc_global;

    if (stored == NULL)
	{	    
	    fprintf (stderr, "Unhandled exception\n");
	    _y_exc_print (stderr, _y_exc_file, _y_exc_function,
			 _y_exc_line, _y_exc_code);
	    
	    raise (SIGABRT);
	}
    
    _y_exc_global = stored->prev;
    
    if (j)
	{
	    /* This assumes that JMP_BUF is a structure etc. and can be
	       copied rawely.  This is true in GLIBC, as I know. */
	    memcpy (j, &stored->j, sizeof (jmp_buf));
	}
        
    /* While with MALLOC, free.  When using obstacks it is better not to
       free and hold up. */
    free (stored);
}

extern "C" void _y_exc_store (TypeInfo* info, void* data) {
    _y_exc_type_info = info;
    _y_exc_type_data = data;
}

extern "C" void* _y_exc_check_type (TypeInfo * info) {
    if (info-> vtable-> equals (info, *_y_exc_type_info)) {
	return _y_exc_type_data;
    }
    return NULL;
}

extern "C" void _y_exc_throw (char *file, char *function, unsigned line, TypeInfo* info, void* data)
{
    jmp_buf j;

    _y_exc_file = file;
    _y_exc_function = function;
    _y_exc_line = line;

    /* Pop for jumping. */
    _y_exc_pop (&j);
    _y_exc_store (info, data);
   
    /* LONGJUMP to J with nonzero value. */
    longjmp (j, 1);
}

extern "C" void _y_exc_rethrow () {
    jmp_buf j;
    _y_exc_pop (&j);
   
    /* LONGJUMP to J with nonzero value. */
    longjmp (j, 1);
}

#include <setjmp.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "../include/throw.h"
#include "../include/print.h"


/* This is the global stack of catchers. */
struct _yrt_thread_stack *_yrt_exc_global = NULL;
pthread_mutex_t _yrt_exc_mutex;

_yrt_array_ _yrt_exc_resolve_stack_trace (_yrt_array_ syms);

void _yrt_exc_init () {
    _yrt_thread_mutex_init (&_yrt_exc_mutex, NULL);
}


struct _yrt_thread_stack * _yrt_get_thread_stack_current (pthread_t id, struct _yrt_thread_stack * current) {
    if (current != NULL) {
	if (pthread_equal (current-> id, id)) return current;
	else {
	    return _yrt_get_thread_stack_current (id, current-> next);
	}
    }
    return NULL;
}

struct _yrt_thread_stack * _yrt_insert_thread (pthread_t id) {    
    struct _yrt_thread_stack * head;
    head = (struct _yrt_thread_stack*) malloc (sizeof (struct _yrt_thread_stack));
    head-> id = id;
    head-> data = NULL;
    head-> stack = NULL;

    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    head-> next = _yrt_exc_global;
    _yrt_exc_global = head;
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);

    return head;
}

struct _yrt_thread_stack * _yrt_get_thread_stack (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    struct _yrt_thread_stack * got = _yrt_get_thread_stack_current (id, _yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
    if (got == NULL) {
	return _yrt_insert_thread (id);
    } else return got;
}

struct _yrt_thread_stack * _yrt_get_thread_stack_no_add (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    struct _yrt_thread_stack * got = _yrt_get_thread_stack_current (id, _yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
    
    return got;
}

void _yrt_remove_thread_current (pthread_t id, struct _yrt_thread_stack * current,  struct _yrt_thread_stack ** last) {
    if (current != NULL) {
	if (current-> id == id) {
	    *last = current-> next;
	    free (current);
	} else {
	    _yrt_remove_thread_current (id, current-> next, &((*(last))-> next));
	}
    }
}

void _yrt_remove_thread (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_remove_thread_current (id, _yrt_exc_global, &_yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
}

int _yrt_exc_push (jmp_buf * j, int returned) {
    if (returned != 0) {
	return 0;
    }
        
    pthread_t id = pthread_self ();
    struct _yrt_thread_stack * _list_ = _yrt_get_thread_stack (id);
    
    struct _yrt_exc_stack * head = (struct _yrt_exc_stack*) malloc (sizeof (struct _yrt_exc_stack));
    memcpy (&head-> j, j, sizeof (jmp_buf));
    head->next = _list_-> stack;
    _list_-> stack = head;
    return 1;
}

void _yrt_exc_pop_list (jmp_buf * j, struct _yrt_thread_stack * _list_) {
    if (_list_ == NULL || _list_-> stack == NULL) {
	fprintf (stderr, "Unhandled exception\n");
	_yrt_exc_print (stderr, _list_);
	    
	raise (SIGABRT);
    }

    memcpy (j, &(_list_-> stack-> j), sizeof (jmp_buf));
    struct _yrt_exc_stack * stored = _list_-> stack;	
    _list_-> stack = _list_-> stack-> next;
    free (stored);

    if (_list_-> stack == NULL && _list_-> data == NULL) {
    	_yrt_remove_thread (_list_-> id);
    }
}

_yrt_array_ _yrt_exc_get_stack_trace ();

void _yrt_exc_pop (jmp_buf * j) {
    pthread_t id = pthread_self ();
    struct _yrt_thread_stack * _list_ = _yrt_get_thread_stack_no_add (id);    
    _yrt_exc_pop_list (j, _list_);
}


void* _yrt_exc_check_type (_ytype_info info) {
    pthread_t id = pthread_self ();
    struct _yrt_thread_stack * _list_ = _yrt_get_thread_stack_no_add (id);
    if (_list_ != NULL) {
	if (_yrt_type_equals (_list_-> info, info)) {
	    void * ret = _list_-> data;
	    _list_-> data = NULL;
	    return ret;
	}
    }
    return NULL;
}

void _yrt_exc_panic (char *file, char *function, unsigned line);

void _yrt_exc_throw (char *file, char *function, unsigned line, _ytype_info info, void* data)
{
    jmp_buf j;
    pthread_t id = pthread_self ();
    struct _yrt_thread_stack * _list_ = _yrt_get_thread_stack (id); 
    
    _list_-> file = file;
    _list_-> function = function;
    _list_-> line = line;
    _list_-> info = info;
    _list_-> data = data;
    
    /* Pop for jumping. */
    _yrt_exc_pop_list (&j, _list_);

    /* LONGJUMP to J with nonzero value. */
    longjmp (j, 1);    
}

void _yrt_exc_rethrow () {
    jmp_buf j;   

    /* Pop for jumping. */
    _yrt_exc_pop (&j);
   
    /* LONGJUMP to J with nonzero value. */
    longjmp (j, 1);
}

void _yrt_exc_print (FILE *stream, struct _yrt_thread_stack * list)
{
    fprintf (stream, "Exception in file \"%s\", at line %u", list-> file, list-> line);
    
    if (list-> function) {
	fprintf (stream, ", in function \"%s\"", list-> function);
    }
    
    fprintf (stream, ", of type ");    
    int * data = (int*) list-> info.name.data;
    for (unsigned int i = 0 ; i < list-> info.name.len ; i++) {
	char c[5];
	int nb = 0;
	fprintf (stream, "%s", _yrt_to_utf8 (data [i], c, &nb));
    }
    
    fprintf (stream, ".");
    
#ifdef _yrt_EXC_PRINT
    fprintf (stream, " Exception ");
    _yrt_EXC_PRINT (list-> code, stream);
#endif
    fprintf (stream, "\n");

    _yrt_array_ trace = _yrt_exc_get_stack_trace ();
    if (trace.len != 0) {
	trace = _yrt_exc_resolve_stack_trace (trace);
	if (trace.len != 0) {
	    fprintf (stream, "%s\n", (char*) trace.data);
	}
    }
}

#include <setjmp.h>
#include <unistd.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <type.hh>
#include <pthread.h>
#include <throw.hh>
#include <print.hh>


void fprintPt(FILE *f, pthread_t pt) {
    unsigned char *ptc = (unsigned char*)(void*)(&pt);
    fprintf(f, "0x");
    for (size_t i=0; i<sizeof(pt); i++) {
	fprintf(f, "%02x", (unsigned)(ptc[i]));
    }
}

extern "C" void printS (struct _yrt_exc_stack * current, int i) {
    if (current != NULL) {
	printf ("%d::", i);
	printS (current-> next, i += 1);
    }
}

extern "C" void print (struct _yrt_thread_stack * current) {
    if (current != NULL) {
	fprintPt (stdout, current-> id);
	printf ("(");
	printS (current-> stack, 0);
	printf (")::");
	print (current-> next);
    }
}

/* This is the global stack of catchers. */
struct _yrt_thread_stack *_yrt_exc_global = NULL;
pthread_mutex_t _yrt_exc_mutex;

extern "C" void _yrt_exc_init () {
    //    _yrt_thread_mutex_init (&_yrt_exc_mutex, NULL);
}


extern "C" _yrt_thread_stack * _yrt_get_thread_stack_current (pthread_t id, struct _yrt_thread_stack * current) {
    if (current != NULL) {
	if (pthread_equal (current-> id, id)) return current;
	else {
	    return _yrt_get_thread_stack_current (id, current-> next);
	}
    }
    return NULL;
}

extern "C" _yrt_thread_stack * _yrt_insert_thread (pthread_t id) {    
    _yrt_thread_stack * head;
    head = (_yrt_thread_stack*) malloc (sizeof (_yrt_thread_stack));
    head-> id = id;
    head-> data = NULL;
    head-> stack = NULL;

    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    head-> next = _yrt_exc_global;
    _yrt_exc_global = head;    
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);

    return head;
}

extern "C" _yrt_thread_stack * _yrt_get_thread_stack (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_thread_stack * got = _yrt_get_thread_stack_current (id, _yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
    if (got == NULL) {
	return _yrt_insert_thread (id);
    } else return got;
}

extern "C" _yrt_thread_stack * _yrt_get_thread_stack_no_add (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_thread_stack * got = _yrt_get_thread_stack_current (id, _yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
    
    return got;
}

extern "C" void _yrt_remove_thread_current (pthread_t id, struct _yrt_thread_stack * current, struct _yrt_thread_stack ** last) {
    if (current != NULL) {
	if (current-> id == id) {
	    *last = current-> next;
	    free (current);
	} else {
	    _yrt_remove_thread_current (id, current-> next, &((*(last))-> next));
	}
    }
}

extern "C" void _yrt_remove_thread (pthread_t id) {
    _yrt_thread_mutex_lock (&_yrt_exc_mutex);
    _yrt_remove_thread_current (id, _yrt_exc_global, &_yrt_exc_global);
    _yrt_thread_mutex_unlock (&_yrt_exc_mutex);
}

extern "C" int _yrt_exc_push (jmp_buf * j, int returned) {
    if (returned != 0) {
	return 0;
    }
    
    pthread_t id = pthread_self ();
    _yrt_thread_stack * _list_ = _yrt_get_thread_stack (id);
    
    _yrt_exc_stack * head = (_yrt_exc_stack*) malloc (sizeof (_yrt_exc_stack));
    memcpy (&head-> j, j, sizeof (jmp_buf));
    head->next = _list_-> stack;
    _list_-> stack = head;
    return 1;
}

extern "C" void _yrt_exc_pop_list (jmp_buf * j, struct _yrt_thread_stack * _list_) {
    if (_list_-> stack == NULL) {
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

extern "C" void _yrt_exc_pop (jmp_buf * j) {
    pthread_t id = pthread_self ();
    struct _yrt_thread_stack * _list_ = _yrt_get_thread_stack_no_add (id);
    _yrt_exc_pop_list (j, _list_);
}


extern "C" void* _yrt_exc_check_type (TypeInfo info) {
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

extern "C" void _yrt_exc_throw (char *file, char *function, unsigned line, TypeInfo info, void* data)
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

extern "C" void _yrt_exc_rethrow () {
    jmp_buf j;   
    
    /* Pop for jumping. */
    _yrt_exc_pop (&j);
   
    /* LONGJUMP to J with nonzero value. */
    longjmp (j, 1);
}


extern "C" void _yrt_exc_print (FILE *stream, struct _yrt_thread_stack * list)
{
    fprintf (stream, "Exception in file \"%s\", at line %u", list-> file, list-> line);
    
    if (list-> function) {
	fprintf (stream, ", in function \"%s\"", list-> function);
    }
    
    fprintf (stream, ", of type ");    
    int * data = (int*) list-> info.name.data;
    for (unsigned int i = 0 ; i < list-> info.name.len ; i++) {
	char c[5];
	fprintf (stream, "%s", toUtf8 (data [i], c));
    }
    
    fprintf (stream, ".");
    
#ifdef _yrt_EXC_PRINT
    fprintf (stream, " Exception ");
    _yrt_EXC_PRINT (list-> code, stream);
#endif
    fprintf (stream, "\n");
}

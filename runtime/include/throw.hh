#pragma once
#include <setjmp.h>
#include <type.hh>
#include <thread.hh>

    
/* Stack is actually a linked list of catcher cells. */
struct _yrt_exc_stack
{
    jmp_buf j;
    struct _yrt_exc_stack *next;
};

/** List of all thread (catching exception at some point, or throwing) */
struct _yrt_thread_stack
{
    pthread_t id;
    struct _yrt_thread_stack * next;
    struct _yrt_exc_stack * stack;
    
    TypeInfo info;
    void* data;

    char * file;
    char * function;
    unsigned line;
    int code;
};

extern "C" void _yrt_exc_print (FILE * stream, struct _yrt_thread_stack * list);
extern "C" void _yrt_exc_init ();

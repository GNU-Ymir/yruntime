#pragma once
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned long ulong;

struct OutBuffer  {
    char * current = NULL;
    ulong len = 0;
    ulong capacity = 0;

};

void write_ (OutBuffer&, const char * str);

void write_ (OutBuffer&, int);
	
void write_ (OutBuffer&, long);

void write_ (OutBuffer&, ulong);

void write_ (OutBuffer&, float);
	
void write_ (OutBuffer&, char);

void write_ (OutBuffer&, double);
		
void write_ (OutBuffer&, bool);
	
void resize (OutBuffer&, ulong len);	

template <typename F> 
void writeln (OutBuffer& buf, F f) {
    write_ (buf, f);
    write_ (buf, '\n');
}

template <typename F> 
void write (OutBuffer& buf, F f) {
    write_ (buf, f);
}

    

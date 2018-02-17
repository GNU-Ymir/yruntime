#pragma once
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint;
typedef unsigned long ulong;

class OutBuffer  {
private :
	
    char * current = NULL;
    ulong len = 0;
    ulong capacity = 0;
	
public:

    template <typename ... T>
    OutBuffer (T ... args) {
	write (args ...);
    }
	
    void writef (const char* s) {
	write (s);
    }
	
    template <typename F, typename ... T>
    void writef (const char * s, F f, T ... args) {
	mwritef (s, f, args...);
    }
	
    template <typename F, typename ... T>
    void write (F f, T ... args) {
	write_ (f);
	write (args...);
    }

    template <typename F, typename ... T>
    void writeln (F f, T ... args) {
	write_ (f);
	write (args...);
	write ('\n');
    }

    template <typename F> 
    void writeln (F f) {
	write (f);
	write ('\n');
    }

    char * data ();
	
private :

    void mwritef (const char * s) {
	write (s);
    }	

    template <typename W,  typename ... T>
    void writeMult (const char *& s, int nb, W what, T ... args) {
	for (int i = 0 ; i < nb; i++)
	    write (what);
	mwritef (s, args...);
    }
	
    template <typename F, typename ... T>
    void mwritef (const char *& s, F f, T ... args) {
	while (*s) {
	    if (*s == '\\') {
		++s ;
	    } else if (*s == '%') {
		s++;
		if (*(s + 1) == '*') {
		    s++;
		    writeMult (s, f, args...);
		} else {			
		    write (f);
		    mwritef (s, args...);
		}
		break;
	    }
	    write (*s);
	    s++;
	}
    }
		
    void write ();

    void write_ (const char * str);

    void write_ (int);
	
    void write_ (long);

    void write_ (ulong);

    void write_ (float);
	
    void write_ (char);

    void write_ (double);
		
    void write_ (bool);
	
private:

    void resize (ulong len);
	
};
    
template <typename ... T>
void println (T ... args) {
    OutBuffer buf (args...);
    printf ("%s\n", buf.data ());
    free (buf.data ());	  
}

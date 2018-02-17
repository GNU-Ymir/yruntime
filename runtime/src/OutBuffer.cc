#include <OutBuffer.hh>
#include <string.h>

void OutBuffer::write_ (const char* cs) {
    auto cslen = strlen (cs);
    if (capacity < len + cslen) {
	resize (len + cslen);
    }

    for (uint i = len, j = 0 ; j < cslen ; i++, j++) {
	this-> current [i] = cs [j]; 
    }
	
    len += cslen;
}
       
void OutBuffer::write_ (ulong nb) {
    auto len = snprintf (NULL, 0, "%lu", nb);
    if (this-> capacity < this-> len + len) {
	resize (this-> len + len);
    }

    sprintf (this-> current + this-> len, "%lu", nb);
    this-> len += len;
}

void OutBuffer::write_ (long nb) {
    auto len = snprintf (NULL, 0, "%ld", nb);
    if (this-> capacity < this-> len + len) {
	resize (this-> len + len);
    }

    sprintf (this-> current + this-> len, "%ld", nb);
    this-> len += len;
}

void OutBuffer::write_ (int nb) {
    auto len = snprintf (NULL, 0, "%d", nb);
    if (this-> capacity < this-> len + len) {
	resize (this-> len + len);
    }

    sprintf (this-> current + this-> len, "%d", nb);
    this-> len += len;
}

    
void OutBuffer::write_ (double nb) {
    auto len = snprintf (NULL, 0, "%A", nb);
    if (this-> capacity < this-> len + len) {
	resize (this-> len + len);
    }

    sprintf (this-> current + this-> len, "%A", nb);
    this-> len += len;
}
    
void OutBuffer::write_ (char c) {
    if (this-> capacity < this-> len + 1) {
	resize (this-> len + 1);
    }

    this-> current [this-> len] = c;
    len += 1;
}
    

void OutBuffer::write_ (bool b) {
    if (b)
	this-> write_ ("true");
    else this-> write_ ("false");
}

void OutBuffer::write_ (float nb) {
    auto len = snprintf (NULL, 0, "%A", nb);
    if (this-> capacity < this-> len + len) {
	resize (this-> len + len);
    }

    sprintf (this-> current + this-> len, "%A", nb);
    this-> len += len;
}
    
void OutBuffer::resize (ulong len) {
    if (capacity == 0) capacity = len + 1;
    else if (capacity * 2 < len) capacity = len + capacity + 1;
    else capacity = (capacity * 2) + 1;
	
    char* aux = (char*) malloc (sizeof(char) * capacity);

    for (uint i = 0 ; i < this-> len ; i ++)
	aux [i] = this-> current [i];
	
    free (this-> current);
    this-> current = aux;
}
    

void OutBuffer::write () {}


char * OutBuffer::data () {
    return this-> current;
}


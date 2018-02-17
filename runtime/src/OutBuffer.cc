#include <OutBuffer.hh>
#include <string.h>

void write_ (OutBuffer& buf, const char* cs) {
    auto cslen = strlen (cs);
    if (buf.capacity < buf.len + cslen) {
	resize (buf, buf.len + cslen);
    }

    for (uint i = buf.len, j = 0 ; j < cslen ; i++, j++) {
	buf.current [i] = cs [j]; 
    }
	
    buf.len += cslen;
}
       
void write_ (OutBuffer& buf, ulong nb) {
    auto len = snprintf (NULL, 0, "%lu", nb);
    if (buf.capacity < buf.len + len) {
	resize (buf, buf.len + len);
    }

    sprintf (buf.current + buf.len, "%lu", nb);
    buf.len += len;
}

void write_ (OutBuffer& buf, long nb) {
    auto len = snprintf (NULL, 0, "%ld", nb);
    if (buf.capacity < buf.len + len) {
	resize (buf, buf.len + len);
    }

    sprintf (buf.current + buf.len, "%ld", nb);
    buf.len += len;
}

void write_ (OutBuffer& buf, int nb) {
    auto len = snprintf (NULL, 0, "%d", nb);
    if (buf.capacity < buf.len + len) {
	resize (buf, buf.len + len);
    }

    sprintf (buf.current + buf.len, "%d", nb);
    buf.len += len;
}

    
void write_ (OutBuffer& buf, double nb) {
    auto len = snprintf (NULL, 0, "%A", nb);
    if (buf.capacity < buf.len + len) {
	resize (buf, buf.len + len);
    }

    sprintf (buf.current + buf.len, "%A", nb);
    buf.len += len;
}
    
void write_ (OutBuffer& buf, char c) {
    if (buf.capacity < buf.len + 1) {
	resize (buf, buf.len + 1);
    }

    buf.current [buf.len] = c;
    buf.len += 1;
}
    

void write_ (OutBuffer& buf, bool b) {
    if (b)
	write_ (buf, "true");
    else write_ (buf, "false");
}

void write_ (OutBuffer& buf, float nb) {
    auto len = snprintf (NULL, 0, "%A", nb);
    if (buf.capacity < buf.len + len) {
	resize (buf, buf.len + len);
    }

    sprintf (buf.current + buf.len, "%A", nb);
    buf.len += len;
}
    
void resize (OutBuffer& buf, ulong len) {
    if (buf.capacity == 0) buf.capacity = len + 1;
    else if (buf.capacity * 2 < len) buf.capacity = len + buf.capacity + 1;
    else buf.capacity = (buf.capacity * 2) + 1;
	
    char* aux = (char*) malloc (sizeof(char) * buf.capacity);

    for (uint i = 0 ; i < buf.len ; i ++)
	aux [i] = buf.current [i];
	
    free (buf.current);
    buf.current = aux;
}
    


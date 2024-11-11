#include <rt/memory/print.h>
#include <stdarg.h>
#include <stdio.h>

#include <rt/utils/gc.h>
#include <rt/memory/types.h>
#include <rt/memory/conv.h>

void _yrt_putwchar (uint32_t code) {
    char c[5];
    int nb = 0;
    printf ("%s", _yrt_to_utf8 (code, c, &nb));
}

void _yrt_eputwchar (uint32_t code) {
    char c[5];
    int nb = 0;
    fprintf (stderr, "%s", _yrt_to_utf8 (code, c, &nb));
}

void _yrt_printf32 (float x) {
    if (x > 1.e6f || x < -1.e6f) {
        printf ("%e", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        printf ("%e", x);
    } else
        printf ("%.6g", x);
}

void _yrt_printf64 (double x) {
    if (x > 1.e6f || x < -1.e6f) {
        printf ("%le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        printf ("%le", x);
    } else {
        printf ("%.6lg", x);
    }
}

void _yrt_printf80 (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        printf ("%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        printf ("%Le", x);
    } else {
        printf ("%.6Lg", x);
    }
}

void _yrt_printfsize (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        printf ("%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        printf ("%Le", x);
    } else {
        printf ("%.6Lg", x);
    }
}

void _yrt_eprintf32 (float x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (stderr, "%e", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (stderr, "%e", x);
    } else
        fprintf (stderr, "%.6g", x);
}

void _yrt_eprintf64 (double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (stderr, "%le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (stderr, "%le", x);
    } else {
        fprintf (stderr, "%.6lg", x);
    }
}

void _yrt_eprintf80 (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (stderr, "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (stderr, "%Le", x);
    } else {
        fprintf (stderr, "%.6Lg", x);
    }
}

void _yrt_eprintfsize (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (stderr, "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (stderr, "%Le", x);
    } else {
        fprintf (stderr, "%.6Lg", x);
    }
}

void _yrt_print_error (char * format) {
    fprintf (stderr, "%s", format);
}

uint32_t _yrt_getwchar () {
    char c[5];
    int ig = scanf ("%c", &c[0]);
    size_t size = utf8_codepoint_size (c[0]);
    for (int i = 1 ; i < (int) size ; i++) {
	int ig_ = scanf ("%c", &c[i]);
    }

    size_t nb;
    return _yrt_to_utf32 (c, &nb);
}

uint32_t _yrt_getwchar_in_file (FILE * file) {
    char c[5];
    c[0] = fgetc (file);
    size_t size = utf8_codepoint_size (c[0]);
    for (int i = 1 ; i < (int) size; i ++)
	c[i + 1] = fgetc (file);

    size_t nb;
    return _yrt_to_utf32 (c, &nb);
}

void _yrt_fflush_stdout () {
    fflush (stdout);
}

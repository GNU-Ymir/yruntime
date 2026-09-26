#include <rt/memory/print.h>
#include <stdarg.h>
#include <stdio.h>

#include <rt/utils/gc.h>
#include <rt/memory/types.h>
#include <rt/memory/conv.h>
#include <rt/memory/capture.h>

void _yrt_putwchar (uint32_t code) {
    char c[5];
    int nb = 0;
    fprintf (_yrt_stdout (), "%s", _yrt_i_to_utf8 (code, c, &nb));
}

void _yrt_eputwchar (uint32_t code) {
    char c[5];
    int nb = 0;
    fprintf (_yrt_stderr (), "%s", _yrt_i_to_utf8 (code, c, &nb));
}

void _yrt_printf32 (float x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stdout (), "%e", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stdout (), "%e", x);
    } else
        fprintf (_yrt_stdout (), "%.6g", x);
}

void _yrt_printf64 (double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stdout (), "%le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stdout (), "%le", x);
    } else {
        fprintf (_yrt_stdout (), "%.6lg", x);
    }
}

void _yrt_printf80 (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stdout (), "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stdout (), "%Le", x);
    } else {
        fprintf (_yrt_stdout (), "%.6Lg", x);
    }
}

void _yrt_printfsize (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stdout (), "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stdout (), "%Le", x);
    } else {
        fprintf (_yrt_stdout (), "%.6Lg", x);
    }
}

void _yrt_eprintf32 (float x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stderr (), "%e", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stderr (), "%e", x);
    } else
        fprintf (_yrt_stderr (), "%.6g", x);
}

void _yrt_eprintf64 (double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stderr (), "%le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stderr (), "%le", x);
    } else {
        fprintf (_yrt_stderr (), "%.6lg", x);
    }
}

void _yrt_eprintf80 (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stderr (), "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stderr (), "%Le", x);
    } else {
        fprintf (_yrt_stderr (), "%.6Lg", x);
    }
}

void _yrt_eprintfsize (long double x) {
    if (x > 1.e6f || x < -1.e6f) {
        fprintf (_yrt_stderr (), "%Le", x);
    } else if (x < 1.e-6f && x > -1.e-6f) {
        fprintf (_yrt_stderr (), "%Le", x);
    } else {
        fprintf (_yrt_stderr (), "%.6Lg", x);
    }
}

void _yrt_i_print_error (char * format) {
    fprintf (stderr, "%s", format);
}


void _yrt_fflush_stdout () {
    fflush (_yrt_stdout ());
}

#ifndef PRINT_H_
#define PRINT_H_

#include <rt/memory/types.h>
#include <stdio.h>

void _yrt_putwchar (uint32_t code);
void _yrt_eputwchar (uint32_t code);

void _yrt_printf32 (float x);
void _yrt_printf64 (double x);
void _yrt_printf80 (long double x);
void _yrt_printfsize (long double x);

void _yrt_eprintf32 (float x);
void _yrt_eprintf64 (double x);
void _yrt_eprintf80 (long double x);
void _yrt_eprintfsize (long double x);
void _yrt_print_error (char * format);

void _yrt_fflush_stdout ();


#endif // PRINT_H_

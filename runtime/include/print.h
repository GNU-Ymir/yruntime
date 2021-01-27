#ifndef _PRINT_H_
#define _PRINT_H_

/**
 * Trasnform a int in utf32 format into a utf8 char*
 * @return: chars
 */
char * _yrt_to_utf8 (unsigned int code, char chars [5], int* nb);

/**
 * Transform a utf8 text into utf32 char
 */
unsigned int _yrt_to_utf32 (char* text);

/**
 * Print a utf8 char code in stdout
 */
void _yrt_putwchar (unsigned int code);

/**
 * Print a f32 in stdout
 */
void _yrt_printf32 (float x);

/**
 * Get a wchar from stdin
 */
unsigned int _yrt_getwchar ();

/**
 * read a wchar from a file
 */
unsigned int _yrt_getwchar_in_file (FILE * file);

#endif

#ifndef _RUN_H_
#define _RUN_H_

extern "C" int y_run_main (int argc, char ** argv, int(*y_main) (Array));
extern "C" void _y_error (unsigned long len, char* ptr);


#endif

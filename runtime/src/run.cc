#include <array.hh>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <OutBuffer.hh>
#include <throw.hh>
#include <gc/gc.h>

extern "C" void _yrt_throw_seg_fault ();


bool DEBUG = false;

void runCommand (OutBuffer & buf, char *sys) {
    auto fp = popen (sys, "r");
    if (fp != NULL) {       
	char path[255];
	memset (path, 0, 255 - 1);
    
	write_ (buf, " in function : ");
	auto func = fgets (path, 255 - 1, fp);
	write_ (buf, func);
	char* ret = fgets (path, 255 -1, fp);
	if (path [0] != '?' && ret != NULL) {
	    write_ (buf, "└──>");
	    write_ (buf, path);
	}
	pclose (fp);
    }
}

extern "C" Array _yrt_exc_get_stack_trace () {
    if (DEBUG) {	
	void *trace[16];
	char **messages = (char **)NULL;

	auto trace_size = backtrace(trace, 16);
	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	OutBuffer buf;
    
	write_ (buf, "[bt] Execution path:\n");
	for (int i=2; i<trace_size; ++i)
	    {
		write_ (buf, "[bt] #");
		write_ (buf, i - 1);
	    
		/* find first occurence of '(' or ' ' in message[i] and assume
		 * everything before that is the file name. (Don't go beyond 0 though
		 * (string terminator)*/
		size_t p = 0;
		while(messages[i][p] != '(' && messages[i][p] != ' '
		      && messages[i][p] != 0)
		    ++p;

		char syscom[256];
		snprintf(syscom, 256, "addr2line %p -f -e %.*s", trace[i], (int) p, messages[i]);

		runCommand (buf, syscom);
	    }
    
	return {buf.len, buf.current};
    } else {
	return {0, NULL};
    }
}

extern "C" Array _yrt_exc_get_stack_loc () {
    void *trace[16];
    char **messages = (char **)NULL;

    auto trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    OutBuffer buf;
    
    write_ (buf, "[bt] Execution path:\n");
    for (int i=2; i<3; ++i)
    	{
    	    write_ (buf, "[bt] #");
	    write_ (buf, i - 1);
	    
    	    /* find first occurence of '(' or ' ' in message[i] and assume
    	     * everything before that is the file name. (Don't go beyond 0 though
    	     * (string terminator)*/
    	    size_t p = 0;
    	    while(messages[i][p] != '(' && messages[i][p] != ' '
    		  && messages[i][p] != 0)
    		++p;

    	    char syscom[256];
    	    snprintf(syscom, 256, "addr2line %p -f -e %.*s", trace[i], (int) p, messages[i]);

	    runCommand (buf, syscom);
    	}
    return {buf.len, buf.current};
}

void bt_sighandler(int sig, struct sigcontext ctx) {    
    _yrt_throw_seg_fault ();    
}

void installHandler () {
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL); // On seg fault we throw an exception
    // sigaction(SIGUSR1, &sa, NULL);
    // sigaction(SIGABRT, &sa, NULL); // Not abort, because it is the thing happening when a exception is not caught
}

extern "C" void _y_error (unsigned long len, char * ptr) {
    fprintf (stderr, "Assert failure : ");
    for (unsigned long i = 0 ; i < len ; i++) {
	fprintf (stderr, "%c", ptr [i]);
    }
    fprintf (stderr, "\n");
}

extern "C" int _yrt_run_main_debug (int argc, char ** argv, int(*y_main) (Array)) {
    DEBUG = true;
    installHandler ();
    _yrt_exc_init ();
    
    auto args = (Array*) malloc (sizeof (Array) * argc);
    for (int i = 0 ; i < argc ; i++) {
	args [i] = {strlen (argv [i]), argv [i]};
    }
    auto ret = y_main ({(unsigned long) argc, args});
    free (args);
    return ret;
}

extern "C" int _yrt_run_main (int argc, char ** argv, int(*y_main) (Array)) {
    DEBUG = false;
    installHandler ();
    _yrt_exc_init ();
    //auto args = (Array*) malloc (sizeof (Array) * argc);
    // for (int i = 0 ; i < argc ; i++) {
    // 	args [i] = utf8toUtf32 (argv [i]);
    // }
    auto ret = y_main ({(unsigned long) argc, argv});
    //free (args);
    return ret;
}

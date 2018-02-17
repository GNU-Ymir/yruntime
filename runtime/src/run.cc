#include <array.hh>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <OutBuffer.hh>

char* demangle (char * func) {
    //TODO
    return func;
}

void runCommand (char *sys) {
    auto fp = popen (sys, "r");
    if (fp == NULL) {
	println ("Failed to run command ", sys);
	exit (1);
    }
    
    char path[255];
    memset (path, 0, 255 - 1);    
    printf ("in function : ");
    auto func = fgets (path, 255 - 1, fp);
    printf ("%s", demangle (func));
    printf ("\t%s", fgets (path, 255 - 1, fp));    
    pclose (fp);
}

void bt_sighandler(int sig, struct sigcontext ctx) {
    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;

    printf("Got signal %d\n", sig);
    trace_size = backtrace(trace, 16);
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("[bt] Execution path:\n");
    for (i=2; i<trace_size; ++i)
	{
	    printf("[bt] #%d ", i - 1);
	    /* find first occurence of '(' or ' ' in message[i] and assume
	     * everything before that is the file name. (Don't go beyond 0 though
	     * (string terminator)*/
	    size_t p = 0;
	    while(messages[i][p] != '(' && messages[i][p] != ' '
		  && messages[i][p] != 0)
		++p;

	    char syscom[256];
	    snprintf(syscom, 256, "addr2line %p -f -e %.*s", trace[i], (int) p, messages[i]);
	    runCommand (syscom);
	}

    exit(0);
}

void installHandler () {
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_handler = (void (*)(int))bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
}

extern "C" int y_run_main (int argc, char ** argv, int(*y_main) (Array)) {
    installHandler ();
    auto args = (Array*) malloc (sizeof (Array) * argc);
    for (int i = 0 ; i < argc ; i++) {
	args [i] = {strlen (argv [i]), argv [i]};
    }
    auto ret = y_main ({(unsigned long) argc, args});
    free (args);
    return ret;
}
